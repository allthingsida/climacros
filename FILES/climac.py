"""
I started writing climacros in Python first but I hit a deadend due to Qt and its inability to let me really intercept the text.

I stopped this project and switched to writing it in C++

(c) Elias Bachaalany <elias.bachaalany@gmail.com>
"""

import time, os, re
import idaapi
import idc
from   ida_kernwin import Choose

LOCK_FILENAME   = 'cli_macros.lck'
MACROS_FILENAME = os.path.join(idaapi.get_user_idadir(), 'cli_macros.lst')

DEFAULT_MACROS = {
    "$!"  : ("'0x%x' % idc.here()",                             "Current cursor location (0x...)"),
    "$!"  : ("'%x' % idc.here()",                               "Current cursor location"),
    "$["  : ("'0x%x' % SelStart()",                             "Selection start"),
    "$@b" : ("'0x%x' % idc.Byte(idc.here())",                   "Byte value at current cursor location (0x..)"),
    "$@B" : ("'%x' % idc.Byte(idc.here())",                     "Byte value at current cursor location"),
    "$@d" : ("'0x%x' % idc.Dword(idc.here())",                  "Dword value at current cursor location (0x..)"),
    "$@D" : ("'%x' % idc.Dword(idc.here())",                    "Dword value at current cursor location"),
    "$]"  : ("'0x%x' % idc.SelEnd()",                           "Selection end"),
    "$#"  : ("'0x%x' % (idc.SelEnd() - idc.SelStart())",        "Selection size"),
}
"""Default macros"""


# ----------------------------------------------------------------------------
class repl_eval_t(object):
    RE_REPL = re.compile('\$\{(.*?)\}\$')
    """Regular expression to replace inline expressions"""

    def __init__(self):
        self.repl_dict = {}
        """The replacement dictionary for each macro {macro: expr}"""

        self.re_pattern = None
        """The RE for replacing all macros"""


    @staticmethod
    def _repl_eval(m):
        """Callback for the replacement routine"""
        try:
            expr = m.group(1).strip()
            return str(eval(expr))
        except:
            return ''


    def replace(self, text):
        """Replace the inline expressions with their evaluated values"""
        text = self.re_pattern.sub(lambda m: self.repl_dict[re.escape(m.group(0))], text)
        return self.RE_REPL.sub(self._repl_eval, text)

    def begin_update(self):
        self.repl_dict = {}

    def end_update(self):
        self.re_pattern = re.compile("|".join(self.repl_dict.keys()))

    def update(self, macro, expr):
        self.repl_dict[re.escape(macro)] = '${%s}$' % expr


# ----------------------------------------------------------------------------
class qfilelock_t(object):
    """Poor man's file lock"""
    def __init__(self, fn):
        self.lock_file = os.path.join(idaapi.get_user_idadir(), os.path.basename(fn))

    def __enter__(self):
        # Wait for file to be unlocked
        while os.path.exists(self.lock_file):
            time.sleep(0.5)

        # Create an empty file
        open(self.lock_file, 'w').close()

    def __exit__(self, type, value, traceback):
        try:
            os.unlink(self.lock_file)
        except:
            pass


# ----------------------------------------------------------------------------
class CLIMacrosEditor(Choose):
    def __init__(self, title, inline_repl, width = 100, height = 400):
        Choose.__init__(
            self,
            title,
            [ ["Macro", 10], ["Expression", 30], ["Description", 60] ],
            flags = (   Choose.CH_CAN_INS
                      | Choose.CH_CAN_DEL
                      | Choose.CH_CAN_EDIT
                      | Choose.CH_RESTORE
                      | Choose.CH_CAN_REFRESH),
            embedded = False,
            width = width,
            height = height)

        self.inline_repl = inline_repl

        # Load values and prepare all replacement REs
        self.populate_macros()


    def OnGetSize(self):
        return len(self.items)


    def OnGetLine(self, n):
        return self.items[n]


    def OnInsertLine(self, n):
        new_macro = self.AskParams(*self.items[n])
        if new_macro is None:
            return (Choose.NOTHING_CHANGED, n)

        # Load first, then add new macro
        self.add_macro(macro=new_macro[0], expr=new_macro[1], desc=new_macro[2])

        # Populate, don't load again
        self.populate_macros(do_load=False)

        return (Choose.ALL_CHANGED, n)


    def OnDeleteLine(self, n):
        macro_name = self.items[n][0]
        macros = save_load_macros(save=False)
        del macros[macro_name]
        save_load_macros(save=True)

        return [Choose.ALL_CHANGED] + self.adjust_last_item(n)


    def AskParams(self, macro='$!', expr='${idc.here()}$', desc='Current cursor location'):
        macro = idc.AskStr(macro, "Enter macro name")
        if macro is None:
            return None
        expr =  idc.AskStr(expr, "Enter Python expression")
        if expr is None:
            return None

        desc = idc.AskStr(desc, "Enter macro description")
        if desc is None:
            return None

        return (macro, expr, desc)


    def OnEditLine(self, n):
        new_macro = self.AskParams(*self.items[n])
        if new_macro is None:
            return (Choose.NOTHING_CHANGED, n)

        macro = self.items[n][0]

        # Load, delete but don't save
        self.del_macro(macro, do_save=False)

        # Don't load, add, save
        self.add_macro(macro=new_macro[0], expr=new_macro[1], desc=new_macro[2], do_load=False)

        # Update visual representation
        self.populate_macros(do_load=False)

        return (Choose.ALL_CHANGED, n)


    def show(self):
        return self.Show(True) >= 0


    def populate_macros(self, do_load=True):
        if do_load:
            self.load_macros()

        self.items = []
        self.inline_repl.begin_update()

        for macro, (expr, desc) in self.macros.items():
            self.items.append([macro, expr, desc])
            self.inline_repl.update(macro, expr)

        self.inline_repl.end_update()

        


    def add_macro(self, macro, expr, desc, do_load=True):
        try:
            if do_load:
                self.load_macros()
            self.macros[macro] = (expr, desc)
            self.save_macros()
        except:
            pass


    def del_macro(self, macro, do_save=True):
        try:
            self.load_macros()
            del self.macros[macro]
            if do_save:
                self.save_macros()
        except:
            pass


    def save_macros(self):
        with qfilelock_t(LOCK_FILENAME) as _:
            with open(MACROS_FILENAME, 'w') as f:
                for macro, (expr, desc) in self.macros.items():
                    f.write('%s~~%s~~%s\n' % (macro, expr, desc))


    def load_macros(self):
        try:
            with qfilelock_t(LOCK_FILENAME) as _:
                with open(MACROS_FILENAME, 'r') as f:
                    macros = {}
                    for line in f:
                        line = line.strip()
                        macro, expr, desc = line.split('~~')
                        macros[macro] = (expr, desc)

                    self.macros = macros
        except:
            self.macros = DEFAULT_MACROS


def __quick_unload_script():
    print("Unloaded!!")

inline_repl = repl_eval_t()
c = CLIMacrosEditor("CLI Macro Editor", inline_repl)
#c.load_macros()
#print(c.macros)
#c.show()
