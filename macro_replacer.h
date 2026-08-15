// Copyright (c) 2019-2026 Elias Bachaalany
// SPDX-License-Identifier: LicenseRef-Human-Origin-Source-1.0
//
// This file is licensed under the Human-Origin Source License v1.0.
// See LICENSE.

/*
Macro replacement engine. PURE (no IDA SDK): static macro substitution plus
dynamic ${expr}$ evaluation via an injected callback. Injecting the evaluator
keeps this unit testable headlessly; the production instance (which evaluates via
IDA's Python extlang) is defined in macro_editor.cpp.
*/

#pragma once

#include <functional>
#include <map>
#include <regex>
#include <string>

// Replace macros with static patterns and dynamic ${expr}$ expressions.
class macro_replacer_t
{
public:
    using repl_func_t = std::function<std::string(std::string)>;

private:
    static std::regex RE_EVAL;
    std::regex re_replace;

    struct LongerPatternSort
    {
        bool operator()(const std::string &lhs, const std::string &rhs) const
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
    std::string operator()(const char *text);
    std::string operator()(std::string text);

    // Similar to Python's "re.escape()"
    static std::string escape_re(const std::string re_text);

    // Update the macro replacement map
    void begin_update();
    void update(std::string macro, std::string expr);
    void end_update();
};
