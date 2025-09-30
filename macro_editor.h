/*
Macro Editor: Complete macro subsystem for IDA CLI macros

This module contains:
- Macro data structures and default macros
- Macro replacement engine
- Macro editor UI
*/

#pragma once

#include <string>
#include <regex>
#include <map>
#include <functional>
#include "idasdk.h"

//-------------------------------------------------------------------------
// Constants for macro serialization and CLI management
//-------------------------------------------------------------------------
constexpr char IDAREG_CLI_MACROS[] = "CLI_Macros";
constexpr int MAX_CLI_MACROS = 200;
constexpr int MAX_CLIS = 20;
constexpr char SER_SEPARATOR[] = "\x1";

//-------------------------------------------------------------------------
// Macro definition structure
//-------------------------------------------------------------------------
struct macro_def_t
{
    std::string macro;
    std::string expr;
    std::string desc;

    bool operator==(const macro_def_t& rhs) const
    {
        return macro == rhs.macro;
    }

    void to_string(std::string& str) const
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
// Macro Replacement Engine
//-------------------------------------------------------------------------

// Utility class to replace macros with static patterns and dynamic expressions
class macro_replacer_t
{
public:
    using repl_func_t = std::function<std::string(std::string)>;

private:
    static std::regex RE_EVAL;
    std::regex re_replace;

    struct LongerPatternSort
    {
        bool operator()(const std::string& lhs, const std::string& rhs) const
        {
            if (lhs.size() > rhs.size())
                return true;
            else if (lhs.size() < rhs.size())
                return false;
            else
                return lhs < rhs;
        }
    };
    std::map<std::string, std::string, LongerPatternSort> replace_map;

    repl_func_t m_repl_func;

public:
    macro_replacer_t(repl_func_t repl_func);

    // Replace macros in text
    std::string operator()(const char* text);
    std::string operator()(std::string text);

    // Similar to Python's "re.escape()"
    static std::string escape_re(const std::string re_text);

    // Update the macro replacement map
    void begin_update();
    void update(std::string macro, std::string expr);
    void end_update();
};

// Global macro replacer instance
extern macro_replacer_t macro_replacer;

//-------------------------------------------------------------------------
// Macro Editor UI
//-------------------------------------------------------------------------

// Modal macro editor
class macro_editor_t: public chooser_t
{
protected:
    static const uint32 flags_;
    static const int widths_[];
    static const char *const header_[];

    macros_t m_macros;

    // Edit a macro definition using a modal dialog
    // Parameters:
    //   def    - The macro definition to edit
    //   as_new - true if creating new macro, false if editing existing
    // Returns: true if user confirmed changes, false if cancelled
    static bool edit_macro_def(macro_def_t &def, bool as_new);

    // Registry operations
    void reg_del_macro(const macro_def_t &macro);
    void reg_save_macro(const macro_def_t &macro, std::string *ser_out = nullptr);

    // Add a new macro to the list
    macro_def_t *add_macro(macro_def_t macro);

    // Chooser overrides
    bool init() override;
    size_t idaapi get_count() const override;
    void idaapi get_row(
        qstrvec_t *cols,
        int *icon,
        chooser_item_attrs_t *attrs,
        size_t n) const override;

    // Chooser actions
    cbret_t idaapi ins(ssize_t n) override;
    cbret_t idaapi del(size_t n) override;
    cbret_t idaapi edit(size_t n) override;

public:
    macro_editor_t(const char *title_ = "CLI macros editor");

    // Rebuilds the macros list from registry and updates the macro replacer
    void build_macros_list();
};