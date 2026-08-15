// Copyright (c) 2019-2026 Elias Bachaalany
// SPDX-License-Identifier: LicenseRef-Human-Origin-Source-1.0
//
// This file is licensed under the Human-Origin Source License v1.0.
// See LICENSE.

/*
Macro replacement engine implementation. PURE (no IDA SDK) so it can be unit
tested headlessly with an injected evaluator. The regex-replace-with-callback
helper is reused from libidacpp::text.
*/

#include "macro_replacer.h"

#include <cctype>

#include <libidacpp/text/text.hpp>

using libidacpp::text::regex_replace_cb;

//-------------------------------------------------------------------------
// Macro Replacer Implementation
//-------------------------------------------------------------------------

std::regex macro_replacer_t::RE_EVAL = std::regex(R"(\$\{(.+?)\}\$)");

macro_replacer_t::macro_replacer_t(repl_func_t repl_func)
    : m_repl_func(repl_func)
{
}

std::string macro_replacer_t::operator()(const char *text)
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
    for (auto ch : re_text)
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
    for (auto &kv : replace_map)
    {
        re_str.append(escape_re(kv.first));
        re_str.append("|");
    }
    // Get rid of the trailing '|'
    re_str.pop_back();

    // Form the single regular expression
    re_replace = re_str;
}
