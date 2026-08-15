// Copyright (c) 2019-2026 Elias Bachaalany
// SPDX-License-Identifier: LicenseRef-Human-Origin-Source-1.0
//
// This file is licensed under the Human-Origin Source License v1.0.
// See LICENSE.

/*
Macro Editor UI (Ctrl-3 chooser). Persistence lives in macro_store.{h,cpp}, the
registry backend in macro_store_registry.{h,cpp}, and the substitution engine in
macro_replacer.{h,cpp}. This file wires them to the chooser and owns the runtime
macro_replacer instance.
*/

#include "macro_editor.h"

//-------------------------------------------------------------------------
// Global macro replacer instance.
// Expands the dynamic ${expr}$ form by evaluating expr through IDA's Python
// extlang (via libidacpp::expr); the result must be a string.
macro_replacer_t macro_replacer(
    [](std::string expr) -> std::string
    {
        if (auto s = libidacpp::expr::eval_python_string(expr.c_str()))
            return *s;
        return std::move(expr);   // eval failed: leave the expression untouched
    }
);

//-------------------------------------------------------------------------
// Rebuild the runtime replacer's pattern map from the current macro list.
static void sync_replacer(macro_replacer_t &replacer, const std::vector<macro_def_t> &macros)
{
    replacer.begin_update();
    for (const macro_def_t &m : macros)
        replacer.update(m.macro, m.expr);
    replacer.end_update();
}

//-------------------------------------------------------------------------
// Macro Editor UI Implementation
//-------------------------------------------------------------------------

// Static members
const uint32 macro_editor_t::flags_ = CH_MODAL | CH_KEEP | CH_CAN_DEL | CH_CAN_EDIT | CH_CAN_INS | CH_CAN_REFRESH;
const int macro_editor_t::widths_[3] = { 10, 30, 70 };
const char *const macro_editor_t::header_[3] = { "Macro", "Expression", "Description" };

//-------------------------------------------------------------------------
macro_editor_t::macro_editor_t(const char *title_)
    : chooser_t(flags_, qnumber(widths_), widths_, header_, title_),
      m_store(m_backend)
{
}

//-------------------------------------------------------------------------
bool macro_editor_t::edit_macro_def(macro_def_t &def, bool as_new)
{
    static const char form_fmt[] =
        "%s\n"
        "\n"
        "<~M~acro      :q1:0:60::>\n"
        "<~E~xpression :q2:0:60::>\n"
        "<~D~escription:q3:0:60::>\n"
        "\n";

    // All 3 fields are always editable
    int r;
    qstring form;
    form.sprnt(form_fmt, as_new ? "New macro" : "Edit macro");
    qstring macro = def.macro.c_str(), expr = def.expr.c_str(), desc = def.desc.c_str();
    r = ask_form(form.c_str(), &macro, &expr, &desc);

    if (r > 0)
    {
        def.macro = macro.c_str();
        def.expr  = expr.c_str();
        def.desc  = desc.c_str();
        return true;
    }
    return false;
}

//-------------------------------------------------------------------------
bool macro_editor_t::init()
{
    load();
    return true;
}

//-------------------------------------------------------------------------
size_t idaapi macro_editor_t::get_count() const
{
    return m_store.list().size();
}

//-------------------------------------------------------------------------
void idaapi macro_editor_t::get_row(
    qstrvec_t *cols,
    int *icon,
    chooser_item_attrs_t *attrs,
    size_t n) const
{
    const macro_def_t &macro = m_store.list()[n];
    cols->at(0) = macro.macro.c_str();
    cols->at(1) = macro.expr.c_str();
    cols->at(2) = macro.desc.c_str();
}

//-------------------------------------------------------------------------
// Add a new macro
chooser_t::cbret_t idaapi macro_editor_t::ins(ssize_t n)
{
    macro_def_t new_macro;
    while (true)
    {
        if (!edit_macro_def(new_macro, true))
            return cbret_t(n, chooser_base_t::NOTHING_CHANGED);

        if (m_store.add(new_macro))
            break;

        warning("A macro with the name '%s' already exists. Please choose another name!", new_macro.macro.c_str());
    }

    sync_replacer(macro_replacer, m_store.list());
    return cbret_t(0, chooser_base_t::ALL_CHANGED);
}

//-------------------------------------------------------------------------
// Remove a macro from the list
chooser_t::cbret_t idaapi macro_editor_t::del(size_t n)
{
    macro_def_t victim = m_store.list()[n];   // copy before the store mutates
    m_store.remove(victim);

    sync_replacer(macro_replacer, m_store.list());
    return adjust_last_item(n);
}

//-------------------------------------------------------------------------
// Edit the macro
chooser_t::cbret_t idaapi macro_editor_t::edit(size_t n)
{
    const macro_def_t old_macro = m_store.list()[n];   // copy
    macro_def_t edited_macro = old_macro;

    while (true)
    {
        if (!edit_macro_def(edited_macro, false))
            return cbret_t(n, chooser_base_t::NOTHING_CHANGED);

        if (m_store.update(old_macro, edited_macro))
            break;

        warning("A macro with the name '%s' already exists. Please choose another name!", edited_macro.macro.c_str());
    }

    sync_replacer(macro_replacer, m_store.list());
    return cbret_t(n, chooser_base_t::ALL_CHANGED);
}

//-------------------------------------------------------------------------
// Load persisted macros (populate defaults on first run) and sync the replacer.
void macro_editor_t::load()
{
    m_store.load(DEFAULT_MACROS, qnumber(DEFAULT_MACROS));
    sync_replacer(macro_replacer, m_store.list());
}
