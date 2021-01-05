/*
CLI Macros: a plugin that allows you to define and use macros in IDA's command line interfaces

When a CLI is registered, this plugin augments its functionality so it supports user defined macros. The macros expand to hardcoded strings
or to dynamic expressions evaluated in Python.

To expand Python expressions dynamically, encapsulate the string in ${expression}$.
All expressions should resolve to a string (i.e. have a __str__ magic method).

(c) Elias Bachaalany <elias.bachaalany@gmail.com>
*/

#include <type_traits>
#include <string>
#include <algorithm>
#include <regex>
#include <functional>

#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4267 4244)
#endif
#include <ida.hpp>
#include <loader.hpp>
#include <kernwin.hpp>
#include <expr.hpp>
#include <registry.hpp>
#include <diskio.hpp>
#ifdef _MSC_VER
    #pragma warning(pop)
#endif

#include "utils_impl.cpp"

constexpr char IDAREG_CLI_MACROS[] = "CLI_Macros";
constexpr int MAX_CLI_MACROS = 200;

constexpr char SER_SEPARATOR[] = "\x1";

//-------------------------------------------------------------------------
// Macro definition
struct macro_def_t
{
    std::string macro;
    std::string expr;
    std::string desc;

    bool operator==(const macro_def_t &rhs) const
    {
        return macro == rhs.macro;
    }

    void to_string(std::string &str) const
    {
        str = macro + SER_SEPARATOR + expr + SER_SEPARATOR + desc;
    }
};
typedef qvector<macro_def_t> macros_t;

// Default macros
static macro_def_t DEFAULT_MACROS[] =
{
    {"$!",    "${'0x%x' % idc.here()}$",                                              "Current cursor location (0x...)"},
    {"$!!",   "${'%x' % idc.here()}$",                                                "Current cursor location"},
    {"$<",    "${'0x%x' % idc.get_segm_start(idc.here())}$",                          "Current segment start (0x...)"},
    {"$>",    "${'0x%x' % idc.get_segm_end(idc.here())}$",                            "Current segment end (0x...)"},
    {"$<<",   "${'%x' % idc.get_segm_start(idc.here())}$",                            "Current segment start"},
    {"$>>",   "${'%x' % idc.get_segm_end(idc.here())}$",                              "Current segment end"},
    {"$@b",   "${'0x%x' % idc.get_wide_byte(idc.here())}$",                           "Byte value at current cursor location (0x...)" },
    {"$@B",   "${'%x' % idc.get_wide_byte(idc.here())}$",                             "Byte value at current cursor location"},
    {"$@d",   "${'0x%x' % idc.get_wide_dword(idc.here())}$",                          "Dword value at current cursor location (0x...)"},
    {"$@D",   "${'%x' % idc.get_wide_dword(idc.here())}$",                            "Dword value at current cursor location"},
    {"$@q",   "${'0x%x' % idc.get_qword(idc.here())}$",                               "Qword value at current cursor location (0x...)"},
    {"$@Q",   "${'%x' % idc.get_qword(idc.here())}$",                                 "Qword value at current cursor location"},
    {"$*b",   "${'0x%x' % idc.read_dbg_byte(idc.here())}$",                           "Debugger byte value at current cursor location (0x...)" },
    {"$*B",   "${'%x' % idc.read_dbg_byte(idc.here())}$",                             "Debugger byte value at current cursor location"},
    {"$*d",   "${'0x%x' % idc.read_dbg_dword(idc.here())}$",                          "Debugger dword value at current cursor location (0x...)"},
    {"$*D",   "${'%x' % idc.read_dbg_dword(idc.here())}$",                            "Debugger dword value at current cursor location"},
    {"$*q",   "${'0x%x' % idc.read_dbg_qword(idc.here())}$",                          "Debugger qword value at current cursor location (0x...)"},
    {"$*Q",   "${'%x' % idc.read_dbg_qword(idc.here())}$",                            "Debugger qword value at current cursor location"},
    {"$[",    "${'0x%x' % idc.read_selection_start()}$",                              "Selection start (0x...)"},
    {"$]",    "${'0x%x' % idc.read_selection_end()}$",                                "Selection end (0x...)"},
    {"$[[",   "${'%x' % idc.read_selection_start()}$",                                "Selection start"},
    {"$]]",   "${'%x' % idc.read_selection_end()}$",                                  "Selection end"},
    {"$#",    "${'0x%x' % (idc.read_selection_end() - idc.read_selection_start())}$", "Selection size (0x...)"},
    {"$##",   "${'%x' % (idc.read_selection_end() - idc.read_selection_start())}$",   "Selection size"},
    {"$cls",  "${idaapi.msg_clear()}$",                                               "Clears the output window"}
};

//-------------------------------------------------------------------------
// Macro replace and expand via Python expression evaluation
macro_replacer_t macro_replacer(
    [](std::string expr)->std::string
    {
        if (auto py = pylang())
        {
            qstring errbuf;
            idc_value_t rv;
            if (py->eval_expr(&rv, BADADDR, expr.c_str(), &errbuf) && rv.vtype == VT_STR)
                return rv.qstr().c_str();
        }
        return std::move(expr);
    }
);

//-------------------------------------------------------------------------
// Context structure to allow hooking CLIs
struct cli_ctx_t
{
    const cli_t *old_cli;
    cli_t new_cli;
};

#define MAX_CTX 18
static cli_ctx_t g_cli_ctx[MAX_CTX] = {};

//-------------------------------------------------------------------------
// Mechanism to create cli->execute_line() callback with user data
#define DEF_HOOK(n) execute_line_with_ctx_##n
#define IMPL_HOOK(n) \
    static bool idaapi execute_line_with_ctx_##n(const char *line) \
    { \
        std::string repl = macro_replacer(line); \
        return g_cli_ctx[n].old_cli->execute_line(repl.c_str()); \
    } 

IMPL_HOOK(0);  IMPL_HOOK(1);  IMPL_HOOK(2);  IMPL_HOOK(3);  IMPL_HOOK(4);  IMPL_HOOK(5);
IMPL_HOOK(6);  IMPL_HOOK(7);  IMPL_HOOK(8);  IMPL_HOOK(9);  IMPL_HOOK(10); IMPL_HOOK(11);
IMPL_HOOK(12); IMPL_HOOK(13); IMPL_HOOK(14); IMPL_HOOK(15); IMPL_HOOK(16); IMPL_HOOK(17);

static bool (idaapi *g_cli_execute_line_with_ctx[MAX_CTX])(const char *) =
{
    DEF_HOOK(0),  DEF_HOOK(1),  DEF_HOOK(2),  DEF_HOOK(3),  DEF_HOOK(4),  DEF_HOOK(5),
    DEF_HOOK(6),  DEF_HOOK(7),  DEF_HOOK(8),  DEF_HOOK(9),  DEF_HOOK(10), DEF_HOOK(11),
    DEF_HOOK(12), DEF_HOOK(13), DEF_HOOK(14), DEF_HOOK(15), DEF_HOOK(16), DEF_HOOK(17)
};
#undef DEF_HOOK
#undef IMPL_HOOK

// Ignore UI hooks when set
bool g_b_ignore_ui_notification = false;

//-------------------------------------------------------------------------
const cli_t *hook_cli(const cli_t *cli)
{
    for (int i=0; i < qnumber(g_cli_ctx); ++i)
    {
        auto &ctx = g_cli_ctx[i];
        if (ctx.old_cli != nullptr)
            continue;

        ctx.old_cli = cli;
        ctx.new_cli = *cli;
        ctx.new_cli.execute_line = g_cli_execute_line_with_ctx[i];
        return &ctx.new_cli;
    }
    return nullptr;
}

//-------------------------------------------------------------------------
const cli_t *unhook_cli(const cli_t *cli)
{
    for (auto &ctx: g_cli_ctx)
    {
        if (ctx.old_cli != cli)
            continue;

        ctx.old_cli = nullptr;

        return &ctx.new_cli;
    }
    return nullptr;
}

//---------------------------------------------------------------------------
// UI callback to help us capture CLI registration
static ssize_t idaapi ui_callback(void *, int notification_code, va_list va)
{
    switch (notification_code)
    {
        case ui_install_cli:
        {
            // Only capture CLIs requests not originating internally
            if (g_b_ignore_ui_notification)
                break;

            auto cli     = va_arg(va, const cli_t *);
            auto install = bool(va_arg(va, int));

            auto hooked_cli = install ? hook_cli(cli) : unhook_cli(cli);
            if (hooked_cli != nullptr)
            {
                // [Un]install the replacement CLI
                request_install_cli(hooked_cli, install);

                // Do not accept this CLI [un]registration
                return 1;
            }
        }
    }
    // Pass-through...
    return 0;
}

//-------------------------------------------------------------------------
// Modal macro editor
struct macro_editor_t: public chooser_t
{
protected:
    static const uint32 flags_;
    static const int widths_[];
    static const char *const header_[];

    macros_t m_macros;

    static bool edit_macro_def(macro_def_t &def, bool as_new)
    {
        static const char form_fmt[] =
            "%s\n"
            "\n"
            "%s"
            "<~E~xpression :q2:0:60::>\n"
            "<~D~escription:q3:0:60::>\n"
            "\n";

        // A new macro can edit all 3 fields. An existing one cannot change its name.
        int r;
        qstring form;
        form.sprnt(
            form_fmt, 
            as_new ? "New macro" : "Edit macro",
            as_new ? "<~M~acro      :q1:0:60::>\n" : "");
        qstring macro = def.macro.c_str(), expr = def.expr.c_str(), desc = def.desc.c_str();
        if (as_new)
            r = ask_form(form.c_str(), &macro, &expr, &desc);
        else
            r = ask_form(form.c_str(), &expr, &desc);

        if (r > 0)
        {
            if (as_new)
                def.macro = macro.c_str();
            def.expr  = expr.c_str();
            def.desc  = desc.c_str();
            return true;
        }
        return false;
    }

    void reg_del_macro(const macro_def_t &macro)
    {
        std::string ser;
        macro.to_string(ser);
        reg_update_strlist(IDAREG_CLI_MACROS, nullptr, MAX_CLI_MACROS, ser.c_str());
    }

    void reg_save_macro(const macro_def_t &macro, std::string *ser_out = nullptr)
    {
        std::string ser;
        macro.to_string(ser);
        reg_update_strlist(IDAREG_CLI_MACROS, ser.c_str(), MAX_CLI_MACROS);
        if (ser_out != nullptr)
            *ser_out = std::move(ser);
    }

    // Add a new macro
    macro_def_t *add_macro(macro_def_t macro)
    {
        auto &new_macro = m_macros.push_back();
        new_macro       = std::move(macro);
        return &new_macro;
    }

    bool init() override
    {
        build_macros_list();
        return true;
    }

    size_t idaapi get_count() const override
    {
        return m_macros.size();
    }

    void idaapi get_row(
        qstrvec_t *cols,
        int *icon,
        chooser_item_attrs_t *attrs,
        size_t n) const override
    {
        auto &macro = m_macros[n];
        cols->at(0) = macro.macro.c_str();
        cols->at(1) = macro.expr.c_str();
        cols->at(2) = macro.desc.c_str();
    }

    // Add a new script
    cbret_t idaapi ins(ssize_t n) override
    {
        macro_def_t new_macro;
        while (true)
        {
            if (!edit_macro_def(new_macro, true))
                return {};

            auto p = m_macros.find({ new_macro.macro });
            if (p == m_macros.end())
                break;

            warning("A macro with the name '%s' already exists. Please choose another name!", new_macro.macro.c_str());
        }

        reg_save_macro(*add_macro(std::move(new_macro)));

        build_macros_list();
        return cbret_t(0, chooser_base_t::ALL_CHANGED);
    }

    // Remove a script from the list
    cbret_t idaapi del(size_t n) override
    {
        reg_del_macro(m_macros[n]);

        build_macros_list();
        return adjust_last_item(n);
    }

    // Edit the macro
    cbret_t idaapi edit(size_t n) override
    {
        // Take a copy of the old macro
        auto old_macro = m_macros[n];

        // In place edit the macro
        auto &macro = m_macros[n];
        if (!edit_macro_def(macro, false))
            return cbret_t(n, chooser_base_t::NOTHING_CHANGED);

        // Delete the old macro
        reg_del_macro(old_macro);

        // Re-insert the macro with different fields (same macro name)
        reg_save_macro(macro);

        build_macros_list();
        return cbret_t(n, chooser_base_t::ALL_CHANGED);
    }

public:
    macro_editor_t(const char *title_);

    // Rebuilds the macros list
    void build_macros_list()
    {
        // Read all the serialized macro definitions
        qstrvec_t ser_macros;
        reg_read_strlist(&ser_macros, IDAREG_CLI_MACROS);
        m_macros.qclear();

        // Empty macros?
        if (ser_macros.empty())
        {
            // If this is not the first run, then keep the macros list empty
            qstring first_run;
            first_run.sprnt("%s/firstrun.climacros", get_user_idadir());
            if (!qfileexist(first_run.c_str()))
            {
                // Populate with the default macros (once)
                FILE *fp = qfopen(first_run.c_str(), "w"); qfclose(fp);
                for (auto &macro: DEFAULT_MACROS)
                {
                    std::string ser_macro;
                    reg_save_macro(*add_macro(macro), &ser_macro);
                    ser_macros.push_back(ser_macro.c_str());
                }
            }
        }
        else
        {
            for (auto &ser_macro: ser_macros)
            {
                char *macro_str = ser_macro.extract();
                char *sptr;
                int icol = 0;
                macro_def_t macro;
                for (auto tok = qstrtok(macro_str, SER_SEPARATOR, &sptr);
                     tok != nullptr;
                     tok = qstrtok(nullptr, SER_SEPARATOR, &sptr), ++icol)
                {
                    if (icol == 0)      macro.macro = tok;
                    else if (icol == 1) macro.expr  = tok;
                    else if (icol == 2) macro.desc  = tok;
                }
                add_macro(std::move(macro));
                qfree(macro_str);
            }
        }

        // Re-create the pattern replacement
        macro_replacer.begin_update();
        for (auto &m: m_macros)
            macro_replacer.update(m.macro, m.expr);
        macro_replacer.end_update();
    }
};

const uint32 macro_editor_t::flags_ = CH_MODAL | CH_KEEP | CH_CAN_DEL | CH_CAN_EDIT | CH_CAN_INS | CH_CAN_REFRESH;

const int macro_editor_t::widths_[3]         = { 10, 30, 70 };
const char *const macro_editor_t::header_[3] = { "Macro", "Expression", "Description" };

inline macro_editor_t::macro_editor_t(const char *title_ = "CLI macros editor")
        : chooser_t(flags_, qnumber(widths_), widths_, header_, title_)
{
}

macro_editor_t g_macro_editor;

//--------------------------------------------------------------------------
plugmod_t *idaapi init(void)
{
    if (!is_idaq())
        return PLUGIN_SKIP;

    msg("IDA Command Line Interface macros initialized\n");

    hook_to_notification_point(HT_UI, ui_callback);
    g_macro_editor.build_macros_list();

    return PLUGIN_KEEP;
}

//--------------------------------------------------------------------------
void idaapi term(void)
{
    unhook_from_notification_point(HT_UI, ui_callback);
}

//--------------------------------------------------------------------------
bool idaapi run(size_t)
{
    g_macro_editor.choose();
    return true;
}

#ifdef _DEBUG
    static const char wanted_hotkey[] = "Ctrl-Shift-A";
#else
    // No hotkey, just run from the Ctrl+3 dialog
    static const char wanted_hotkey[] = "";
#endif

//--------------------------------------------------------------------------
static const char comment[] = "Use macros in CLIs";
static const char help[]    = 
    "Define your own macros and use then in the CLIs.\n"
    "Comes in handy with the WinDbg or other debuggers' CLIs\n"
    "\n"
    "climacros is developed by Elias Bachaalany. Please see https://github.com/0xeb/ida-climacros for more information\n"
    "\0"
    __DATE__ " " __TIME__ "\n"
    "\n";

//--------------------------------------------------------------------------
//
//      PLUGIN DESCRIPTION BLOCK
//
//--------------------------------------------------------------------------
plugin_t PLUGIN =
{
  IDP_INTERFACE_VERSION,
  PLUGIN_FIX,           // plugin flags: load once and stay until IDA exits
  init,                 // initialize

  term,                 // terminate. this pointer may be NULL.

  run,                  // invoke plugin

  comment,              // long comment about the plugin
                        // it could appear in the status line
                        // or as a hint
  help,                 // multiline help about the plugin

  "CLI Macros",         // the preferred short name of the plugin
  wanted_hotkey         // the preferred hotkey to run the plugin
};
