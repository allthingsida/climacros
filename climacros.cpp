/*
CLI Macros: a plugin that allows you to define and use macros in IDA's command line interfaces

When a CLI is registered, this plugin augments its functionality so it supports user defined macros. The macros expand to hardcoded strings
or to dynamic expressions evaluated in Python.

To expand Python expressions dynamically, encapsulate the string in ${expression}$.
All expressions should resolve to a string (i.e. have a __str__ magic method).

(c) Elias Bachaalany <elias.bachaalany@gmail.com>
*/

#include "idasdk.h"
#include <idax/xcallbacks.hpp>
#include "cli_utils.h"
#include "macro_editor.h"

//-------------------------------------------------------------------------
class climacros_plg_t : public plugmod_t, public event_listener_t
{
    macro_editor_t macro_editor;

public:
    climacros_plg_t() : plugmod_t()
    {
        msg("IDA Command Line Interface macros initialized\n");

        macro_editor.build_macros_list();
        hook_event_listener(HT_UI, this, HKCB_GLOBAL);

        // Hook pre-existing CLIs (like Python) that were loaded before our plugin
        hook_preexisting_clis();
    }

    void hook_preexisting_clis()
    {
        // Tabular approach: define CLI finders to try
        struct cli_finder_t
        {
            const char* name;
            cli_t* (*finder)();
        };

        static const cli_finder_t cli_finders[] = 
        {
            {"Python", find_python_cli},
            {"IDC",    find_idc_cli}
        };

        // Try to find and hook each CLI
        for (const auto& finder : cli_finders)
        {
            cli_t* cli = finder.finder();
            if (cli != nullptr)
            {
                // Hook the CLI
                auto new_cli = hook_cli(cli);
                if (new_cli != nullptr)
                {
                    // Uninstall previous CLI and replace with new hooked CLI
                    request_install_cli(cli, false);
                    request_install_cli(new_cli, true);
                    msg("climacros: successfully hooked pre-existing CLI '%s'\n", cli->sname);
                }
            }
        }
    }

    bool idaapi run(size_t) override
    {
        macro_editor.choose();
        return true;
    }

    ssize_t idaapi on_event(ssize_t code, va_list va) override
    {
        switch (code)
        {
            case ui_install_cli:
            {
                // Only capture CLIs requests not originating internally
                if (g_b_ignore_ui_notification)
                    break;

                auto cli = va_arg(va, const cli_t*);
                auto install = bool(va_arg(va, int));

                if (install)
                {
                    // Create a copy of the CLI with our execute_line hook
                    auto new_cli = hook_cli(cli);
                    // Remove the old CLI and install the new one
                    request_install_cli(cli, false);
                    request_install_cli(new_cli, true);
                    msg("climacros: hooked CLI '%s'\n", cli->sname);
                }
                else
                {
                    // Find the new CLI using the prevously registered CLI
                    // Note: from the original plugin perspective, the old CLI was never uninstalled
                    auto new_cli = unhook_cli(cli);
                    // Remove the new CLI (which we installed when we uninstalled the old CLI)
                    request_install_cli(new_cli, false);
                    msg("climacros: unhooked CLI '%s'\n", cli->sname);
                }
            }
        }
        return 0;
    }

    ~climacros_plg_t()
    {
        unhook_event_listener(HT_UI, this);
    }
};

//--------------------------------------------------------------------------
plugmod_t *idaapi init(void)
{
    if (!is_idaq())
        return nullptr;

    return new climacros_plg_t;
}

#ifdef _DEBUG
    static const char wanted_hotkey[] = "Ctrl-Shift-A";
#else
    // No hotkey, just run from the Ctrl+3 dialog
    static const char wanted_hotkey[] = "";
#endif

//--------------------------------------------------------------------------
static const char comment[] = "Use macros in CLIs";
static const char help[]    =
    "Define your own macros and use then in the CLIs.\n"
    "Comes in handy with the WinDbg or other debuggers' CLIs\n"
    "\n"
    "climacros is developed by Elias Bachaalany. Please see https://github.com/allthingsida/ida-climacros for more information\n"
    "\0"
    __DATE__ " " __TIME__ "\n"
    "\n";

//--------------------------------------------------------------------------
//
//      PLUGIN DESCRIPTION BLOCK
//
//--------------------------------------------------------------------------
plugin_t PLUGIN =
{
  IDP_INTERFACE_VERSION,
  PLUGIN_FIX | PLUGIN_MULTI,
  init,                 // initialize
  nullptr,              // terminate. this pointer may be NULL.

  nullptr,              // invoke plugin

  comment,              // long comment about the plugin
                        // it could appear in the status line
                        // or as a hint
  help,                 // multiline help about the plugin

  "CLI Macros",         // the preferred short name of the plugin
  wanted_hotkey         // the preferred hotkey to run the plugin
};