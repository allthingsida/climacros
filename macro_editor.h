// Copyright (c) 2019-2026 Elias Bachaalany
// SPDX-License-Identifier: LicenseRef-Human-Origin-Source-1.0
//
// This file is licensed under the Human-Origin Source License v1.0.
// See LICENSE.

/*
Macro Editor: the Ctrl-3 chooser UI for managing CLI macros.

This is now a thin UI layer: persistence/serialization live in macro_store.{h,cpp}
(pure) behind a backend (registry_backend_t, macro_store_registry.{h,cpp}), and the
substitution engine lives in macro_replacer.{h,cpp} (pure). The editor only edits
entries and keeps the runtime macro_replacer in sync.
*/

#pragma once

#include "idasdk.h"
#include "macro_store.h"
#include "macro_store_registry.h"
#include "macro_replacer.h"

//-------------------------------------------------------------------------
// Global macro replacer instance (defined in macro_editor.cpp; evaluates the
// dynamic ${expr}$ form via IDA's Python extlang).
extern macro_replacer_t macro_replacer;

//-------------------------------------------------------------------------
// Default macros — the single source of truth. The public README macro table is
// generated from this array by kb-ati/climacros/scripts/gen_macros_readme.py, so
// keep the `{ "trigger", "expr", "desc" }` literal shape and this variable name.
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
    {"$@w",   "${'0x%x' % idc.get_wide_word(idc.here())}$",                           "Word value at current cursor location (0x...)"},
    {"$@W",   "${'%x' % idc.get_wide_word(idc.here())}$",                             "Word value at current cursor location"},
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

    // Function bounds (parens = function)
    {"$(",    "${(lambda f: '0x%x' % f.start_ea if f else '?')(idaapi.get_func(idc.here()))}$", "Current function start (0x...)"},
    {"$)",    "${(lambda f: '0x%x' % f.end_ea if f else '?')(idaapi.get_func(idc.here()))}$",   "Current function end (0x...)"},

    // Navigation ('+' = next item, '-' = previous item)
    {"$+",    "${'0x%x' % idc.next_head(idc.here())}$",                               "Next item address (0x...)"},
    {"$-",    "${'0x%x' % idc.prev_head(idc.here())}$",                               "Previous item address (0x...)"},

    // Module-relative addresses
    {"$^",    "${'0x%x' % (idc.here() - idaapi.get_imagebase())}$",                   "RVA of cursor (offset from image base) (0x...)"},
    {"$_",    "${'0x%x' % idaapi.get_imagebase()}$",                                  "Image base (0x...)"},

    {"$cls",  "${idaapi.msg_clear()}$",                                               "Clears the output window"}
};

//-------------------------------------------------------------------------
// Modal macro editor (Ctrl-3)
//-------------------------------------------------------------------------
class macro_editor_t: public chooser_t
{
protected:
    static const uint32 flags_;
    static const int widths_[];
    static const char *const header_[];

    registry_backend_t m_backend;   // production persistence (IDA registry)
    macro_store_t      m_store;     // in-memory list kept in sync with m_backend

    // Edit a macro definition using a modal dialog. Returns true if confirmed.
    static bool edit_macro_def(macro_def_t &def, bool as_new);

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

    // Load persisted macros (populating defaults on first run) and sync the
    // runtime macro_replacer. Called at plugin init and after every edit.
    void load();
};
