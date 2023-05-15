//-------------------------------------------------------------------------
// Macro definition
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
