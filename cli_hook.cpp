// Copyright (c) 2019-2026 Elias Bachaalany
// SPDX-License-Identifier: LicenseRef-Human-Origin-Source-1.0
//
// This file is licensed under the Human-Origin Source License v1.0.
// See LICENSE.

/*
CLI Hooking implementation: manages a fixed pool of hooked CLIs, routing each
CLI's execute_line() through the global macro_replacer.
*/

#include <string>

#include "idasdk.h"
#include "cli_hook.h"
#include "macro_editor.h"   // global macro_replacer instance

#include <libidacpp/callbacks/callbacks.hpp>

using namespace libidacpp::callbacks;

//-------------------------------------------------------------------------
// Callback registry for CLI execute_line hooks.
DEFINE_CALLBACK_REGISTRY(cli_execute_registry, decltype(cli_t::execute_line), MAX_CLIS)

// Per-slot hook context.
//
// Each slot's registry callback is registered ONCE (lazily) and never
// unregistered: unhooking a CLI only clears old_cli, and re-hooking reuses the
// slot's existing registration. This keeps the fixed-size callback registry from
// being exhausted by CLI install/uninstall churn (e.g. repeated debugger
// sessions), since the number of registrations can never exceed MAX_CLIS.
struct cli_ctx_t
{
    const cli_t *old_cli = nullptr;                    // hooked CLI (nullptr => slot free)
    cli_t new_cli = {};                                // our hooked copy handed to IDA
    decltype(cli_t::execute_line) hook_fn = nullptr;   // registry wrapper for this slot (registered once)
};

static cli_ctx_t g_cli_ctx[MAX_CLIS] = {};

//-------------------------------------------------------------------------
const cli_t *hook_cli(const cli_t *cli)
{
    for (auto &ctx : g_cli_ctx)
    {
        // Find a free slot
        if (ctx.old_cli != nullptr)
            continue;

        // Register this slot's callback once (lazily); reuse it on later hooks.
        if (ctx.hook_fn == nullptr)
        {
            auto result = cli_execute_registry.register_callback(
                [&ctx](const char *line) -> bool {
                    if (ctx.old_cli == nullptr)   // slot was unhooked; do nothing
                        return false;
                    std::string repl = macro_replacer(line);
                    return ctx.old_cli->execute_line(repl.c_str());
                }
            );
            if (!result)
                break;   // registry full: more concurrent CLIs than MAX_CLIS
            ctx.hook_fn = result->second;
        }

        ctx.old_cli = cli;
        ctx.new_cli = *cli;
        ctx.new_cli.execute_line = ctx.hook_fn;
        return &ctx.new_cli;
    }
    return nullptr;
}

//-------------------------------------------------------------------------
const cli_t *unhook_cli(const cli_t *cli)
{
    for (auto &ctx : g_cli_ctx)
    {
        if (ctx.old_cli != cli)
            continue;

        // Free the slot WITHOUT unregistering: the registration is reused on the
        // next hook, so install/uninstall churn never grows the registry.
        ctx.old_cli = nullptr;

        return &ctx.new_cli;
    }
    return nullptr;
}

//-------------------------------------------------------------------------
// Ignore UI hooks when set
bool g_b_ignore_ui_notification = false;

// [Un]install a CLI (asynchronously) using a UI request
void request_install_cli(const cli_t *cli, bool install)
{
    class cli_install_req_t : public ui_request_t
    {
        const cli_t *cli;
        bool install;
    public:
        cli_install_req_t(const cli_t *cli, bool install) : cli(cli), install(install)
        {
        }

        virtual bool idaapi run()
        {
            g_b_ignore_ui_notification = true;
            if (install)
                install_command_interpreter(cli);
            else
                remove_command_interpreter(cli);
            g_b_ignore_ui_notification = false;

            return false;
        }
    };

    execute_ui_requests(
        new cli_install_req_t(cli, install),
        nullptr);
}
