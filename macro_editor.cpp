/*
Macro Editor: Complete macro subsystem implementation

(c) Elias Bachaalany <elias.bachaalany@gmail.com>
*/

#include <algorithm>
#include "macro_editor.h"

//-------------------------------------------------------------------------
// Custom regex_replace with callback (similar to Python's re.sub())
// Based on https://stackoverflow.com/a/37516316
//-------------------------------------------------------------------------
template<class BidirIt, class Traits, class CharT, class UnaryFunction>
static std::basic_string<CharT> regex_replace_cb(
    BidirIt first,
    BidirIt last,
    const std::basic_regex<CharT, Traits> &re,
    UnaryFunction f)
{
    std::basic_string<CharT> s;

    typename std::match_results<BidirIt>::difference_type positionOfLastMatch = 0;
    auto endOfLastMatch = first;

    auto callback = [&](const std::match_results<BidirIt> &match)
    {
        auto positionOfThisMatch = match.position(0);
        auto diff = positionOfThisMatch - positionOfLastMatch;

        auto startOfThisMatch = endOfLastMatch;
        std::advance(startOfThisMatch, diff);

        s.append(endOfLastMatch, startOfThisMatch);
        s.append(f(match));

        auto lengthOfMatch = match.length(0);

        positionOfLastMatch = positionOfThisMatch + lengthOfMatch;

        endOfLastMatch = startOfThisMatch;
        std::advance(endOfLastMatch, lengthOfMatch);
    };

    std::regex_iterator<BidirIt> begin(first, last, re), end;
    std::for_each(begin, end, callback);

    s.append(endOfLastMatch, last);

    return s;
}

template<class Traits, class CharT, class UnaryFunction>
static std::string regex_replace_cb(
    const std::string &s,
    const std::basic_regex<CharT, Traits> &re,
    UnaryFunction f)
{
    return regex_replace_cb(s.cbegin(), s.cend(), re, f);
}

//-------------------------------------------------------------------------
// Macro Replacer Implementation
//-------------------------------------------------------------------------

std::regex macro_replacer_t::RE_EVAL = std::regex(R"(\$\{(.+?)\}\$)");

macro_replacer_t::macro_replacer_t(repl_func_t repl_func)
    : m_repl_func(repl_func)
{
}

std::string macro_replacer_t::operator()(const char* text)
{
    return operator()(std::string(text));
}

std::string macro_replacer_t::operator()(std::string text)
{
    if (!replace_map.empty())
        text = regex_replace_cb(text, re_replace, [this](auto &m) { return replace_map[m.str(0)]; });

    return regex_replace_cb(text, RE_EVAL, [this](auto &m) { return m_repl_func(m.str(1)); });
}

// Similar to Python's "re.escape()"
std::string macro_replacer_t::escape_re(const std::string re_text)
{
    std::string out;
    out.reserve(re_text.size() * 2);
    for (auto ch: re_text)
    {
        if (isalnum(ch))
        {
            out += ch;
            continue;
        }
        else if (ch == 0)
        {
            out += "\\x0";
        }
        else
        {
            out += '\\';
            out += ch;
        }
    }
    return out;
}

void macro_replacer_t::begin_update()
{
    replace_map.clear();
}

void macro_replacer_t::update(std::string macro, std::string expr)
{
    replace_map[macro] = expr;
}

void macro_replacer_t::end_update()
{
    if (replace_map.empty())
        return;

    std::string re_str;
    for (auto &kv: replace_map)
    {
        re_str.append(escape_re(kv.first));
        re_str.append("|");
    }
    // Get rid of the trailing '|'
    re_str.pop_back();

    // Form the single regular expression
    re_replace = re_str;
}

//-------------------------------------------------------------------------
// Global macro replacer instance
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
// Macro Editor UI Implementation
//-------------------------------------------------------------------------

// Static members
const uint32 macro_editor_t::flags_ = CH_MODAL | CH_KEEP | CH_CAN_DEL | CH_CAN_EDIT | CH_CAN_INS | CH_CAN_REFRESH;
const int macro_editor_t::widths_[3] = { 10, 30, 70 };
const char *const macro_editor_t::header_[3] = { "Macro", "Expression", "Description" };

//-------------------------------------------------------------------------
macro_editor_t::macro_editor_t(const char *title_)
    : chooser_t(flags_, qnumber(widths_), widths_, header_, title_)
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
void macro_editor_t::reg_del_macro(const macro_def_t &macro)
{
    std::string ser;
    macro.to_string(ser);
    reg_update_strlist(IDAREG_CLI_MACROS, nullptr, MAX_CLI_MACROS, ser.c_str());
}

//-------------------------------------------------------------------------
void macro_editor_t::reg_save_macro(const macro_def_t &macro, std::string *ser_out)
{
    std::string ser;
    macro.to_string(ser);
    reg_update_strlist(IDAREG_CLI_MACROS, ser.c_str(), MAX_CLI_MACROS);
    if (ser_out != nullptr)
        *ser_out = std::move(ser);
}

//-------------------------------------------------------------------------
// Add a new macro
macro_def_t *macro_editor_t::add_macro(macro_def_t macro)
{
    auto &new_macro = m_macros.push_back();
    new_macro       = std::move(macro);
    return &new_macro;
}

//-------------------------------------------------------------------------
bool macro_editor_t::init()
{
    build_macros_list();
    return true;
}

//-------------------------------------------------------------------------
size_t idaapi macro_editor_t::get_count() const
{
    return m_macros.size();
}

//-------------------------------------------------------------------------
void idaapi macro_editor_t::get_row(
    qstrvec_t *cols,
    int *icon,
    chooser_item_attrs_t *attrs,
    size_t n) const
{
    auto &macro = m_macros[n];
    cols->at(0) = macro.macro.c_str();
    cols->at(1) = macro.expr.c_str();
    cols->at(2) = macro.desc.c_str();
}

//-------------------------------------------------------------------------
// Add a new script
chooser_t::cbret_t idaapi macro_editor_t::ins(ssize_t n)
{
    macro_def_t new_macro;
    while (true)
    {
        if (!edit_macro_def(new_macro, true))
            return cbret_t(n, chooser_base_t::NOTHING_CHANGED);

        auto p = m_macros.find({ new_macro.macro });
        if (p == m_macros.end())
            break;

        warning("A macro with the name '%s' already exists. Please choose another name!", new_macro.macro.c_str());
    }

    reg_save_macro(*add_macro(std::move(new_macro)));

    build_macros_list();
    return cbret_t(0, chooser_base_t::ALL_CHANGED);
}

//-------------------------------------------------------------------------
// Remove a script from the list
chooser_t::cbret_t idaapi macro_editor_t::del(size_t n)
{
    reg_del_macro(m_macros[n]);

    build_macros_list();
    return adjust_last_item(n);
}

//-------------------------------------------------------------------------
// Edit the macro
chooser_t::cbret_t idaapi macro_editor_t::edit(size_t n)
{
    // Take a copy of the old macro
    auto old_macro = m_macros[n];

    // Edit a copy of the macro
    auto edited_macro = m_macros[n];
    while (true)
    {
        if (!edit_macro_def(edited_macro, false))
            return cbret_t(n, chooser_base_t::NOTHING_CHANGED);

        // Check if macro name changed and if new name already exists
        if (edited_macro.macro != old_macro.macro)
        {
            auto p = m_macros.find({ edited_macro.macro });
            if (p != m_macros.end())
            {
                warning("A macro with the name '%s' already exists. Please choose another name!", edited_macro.macro.c_str());
                continue;
            }
        }
        break;
    }

    // Delete the old macro
    reg_del_macro(old_macro);

    // Update the macro in-place and save
    m_macros[n] = edited_macro;
    reg_save_macro(m_macros[n]);

    build_macros_list();
    return cbret_t(n, chooser_base_t::ALL_CHANGED);
}

//-------------------------------------------------------------------------
// Rebuilds the macros list
void macro_editor_t::build_macros_list()
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