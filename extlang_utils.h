// Copyright (c) 2019-2026 Elias Bachaalany
// SPDX-License-Identifier: LicenseRef-Human-Origin-Source-1.0
//
// This file is licensed under the Human-Origin Source License v1.0.
// See LICENSE.

/*
Extlang Utils: hook an external language's eval_snippet() so that macros are
expanded in script *snippets* (e.g. the "Execute script snippet" box), the same
way cli_hook hooks a CLI's execute_line() for the command line.

Unlike CLIs, extlangs expose enumeration (for_all_extlangs) and a lifecycle
event (idb_event::extlang_changed), so no memory scan is needed. We patch the
live extlang_t::eval_snippet function pointer in place (the struct is writable
because IDA ref-counts it), and restore it on removal / teardown.

Only eval_snippet is touched. eval_expr, the selected extlang, and the CLI path
are left alone.
*/

#pragma once

#include <functional>
#include <string>

struct extlang_t;

namespace extlang_hook
{

// Maximum number of concurrently hooked extlangs. Slots are reused, so this caps
// the live count and total lifetime registry usage (never exhausted by churn).
constexpr int MAX_EXTLANGS = 16;

// The macro expander applied to a snippet before it reaches the original
// eval_snippet(). Injected so the engine is testable without the macro editor.
// The plugin wires this to the global macro_replacer; tests inject a fake.
using replacer_t = std::function<std::string(const char *)>;
void set_replacer(replacer_t fn);

// Hook a single extlang's eval_snippet(). Idempotent: hooking an already-hooked
// extlang is a no-op. Returns true if the extlang is hooked afterwards, false if
// it has no eval_snippet() or no free slot is available.
bool hook_extlang(extlang_t *el);

// Restore a previously hooked extlang's eval_snippet() in place and free its
// slot (the slot's registry trampoline is kept for reuse). Returns true if we
// had it hooked, false otherwise.
bool unhook_extlang(extlang_t *el);

// True if we currently hold a hook for this extlang.
bool is_hooked(const extlang_t *el);

// Enumerate all currently-registered extlangs (for_all_extlangs) and hook each
// one that has a non-null eval_snippet(). Use at plugin init for extlangs that
// were registered before we started listening for extlang_changed.
void hook_all_existing();

// Teardown: detach every slot. Intentionally does NOT dereference the hooked
// extlang structs or enumerate extlangs (an owning module may have unloaded), so
// it is safe to call at shutdown. climacros pins its DLL, so the trampolines stay
// mapped and any extlang still pointing at one becomes a harmless no-op. Live
// removals are restored in place by unhook_extlang() on the removed event.
void unhook_all();

} // namespace extlang_hook
