/*
CLI Utils: Header for CLI finding and hooking functionality
*/

#pragma once

struct cli_t;

//-------------------------------------------------------------------------
// CLI Finding Functions
//-------------------------------------------------------------------------

// Find a cli_t structure in a loaded module by searching for a target string in lname field
// Parameters:
//   module_name   - Module name to search in (e.g., "idapython3.dll", "ida.dll")
//   target_string - String to search for in the CLI's lname field
// Returns:
//   Pointer to the cli_t structure if found, nullptr otherwise
cli_t* find_cli_in_module(const char* module_name, const char* target_string);

// Helper: Find Python CLI in IDAPython module
// Searches for "Python - IDAPython plugin" in idapython3.dll/.so/.dylib
cli_t* find_python_cli();

// Helper: Find IDC CLI in IDA main module
// Searches for "IDC - Native built-in language" in ida.dll/.so/.dylib
cli_t* find_idc_cli();

//-------------------------------------------------------------------------
// CLI Hooking Functions
//-------------------------------------------------------------------------

// Hook a CLI's execute_line function to enable macro expansion
// Returns: Pointer to the hooked CLI structure, or nullptr if hooking failed
const cli_t* hook_cli(const cli_t* cli);

// Unhook a previously hooked CLI
// Returns: Pointer to the unhooked CLI structure, or nullptr if not found
const cli_t* unhook_cli(const cli_t* cli);

//-------------------------------------------------------------------------
// CLI Installation
//-------------------------------------------------------------------------

// Ignore UI hooks when set
extern bool g_b_ignore_ui_notification;

// [Un]install a CLI (asynchronously) using a UI request
void request_install_cli(const cli_t *cli, bool install);
