// Copyright (c) 2019-2026 Elias Bachaalany
// SPDX-License-Identifier: LicenseRef-Human-Origin-Source-1.0
//
// This file is licensed under the Human-Origin Source License v1.0.
// See LICENSE.

/*
Extlang Utils: expand macros in script snippets by hooking extlang_t::eval_snippet.

The in-place function-pointer hook (save original / install trampoline / restore,
with reusable slots) is provided by libidacpp::callbacks::inplace_hook. This file
just wires the snippet expander into it and drives discovery via
libidacpp::expr::collect_extlangs.
*/

#include <string>
#include "idasdk.h"
#include "extlang_utils.h"
#include <libidacpp/callbacks/inplace_hook.hpp>
#include <libidacpp/expr/expr.hpp>

namespace extlang_hook
{

// Snippet macro expander (injected). Null => pass the snippet through unchanged.
static replacer_t g_replacer;

// One in-place hook pool for eval_snippet (bool(const char*, qstring*)).
using snippet_hook_t =
    libidacpp::callbacks::inplace_hook<bool(const char *, qstring *), MAX_EXTLANGS, struct climacros_snippet_tag>;
static snippet_hook_t g_hook;

//-------------------------------------------------------------------------
void set_replacer(replacer_t fn)
{
    g_replacer = std::move(fn);
}

//-------------------------------------------------------------------------
bool hook_extlang(extlang_t *el)
{
    if (el == nullptr || el->eval_snippet == nullptr)
        return false;

    return g_hook.hook(&el->eval_snippet,
        [](auto original)
        {
            return [original](const char *str, qstring *errbuf) -> bool
            {
                if (g_replacer)
                {
                    std::string repl = g_replacer(str);
                    return original(repl.c_str(), errbuf);
                }
                return original(str, errbuf);
            };
        });
}

//-------------------------------------------------------------------------
bool unhook_extlang(extlang_t *el)
{
    return el != nullptr && g_hook.unhook(&el->eval_snippet);
}

//-------------------------------------------------------------------------
bool is_hooked(const extlang_t *el)
{
    return el != nullptr && g_hook.is_hooked(&el->eval_snippet);
}

//-------------------------------------------------------------------------
void hook_all_existing()
{
    for (extlang_t *el : libidacpp::expr::collect_extlangs())
        hook_extlang(el);
}

//-------------------------------------------------------------------------
void unhook_all()
{
    // Teardown: detach without dereferencing the extlang structs (an owning module
    // may already be gone). climacros pins its DLL, so the trampolines stay mapped
    // and a leftover call no-ops safely. Live removals are restored in place by
    // unhook_extlang() on the extlang_changed(removed) event.
    g_hook.detach_all();
}

} // namespace extlang_hook
