# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

# climacros - CLI Macros Plugin for IDA Pro

An IDA Pro plugin that enables static and dynamic macro expansion in all command line interfaces (Python, IDC, WinDbg, BochDbg, Gdb, etc.).

## Build System

Always use the **ida-cmake agent** to compile and build this project.

**Build command:**
```bash
cmake --build build --config RelWithDebInfo
```

The plugin is automatically installed to `$IDABIN/plugins/00climacros.dll` (the "00" prefix ensures early loading).

## Architecture

### Core Components

1. **climacros.cpp** - Main plugin implementation
   - `climacros_plg_t`: Plugin module that hooks UI events to intercept CLI registrations
   - `macro_editor_t`: Modal chooser for managing macro definitions
   - CLI hooking system: Intercepts `execute_line()` calls on all registered CLIs to perform macro expansion

2. **utils_impl.cpp** - Utility functions (included directly via `#include` in climacros.cpp:20)
   - `macro_replacer_t`: Core regex-based pattern replacement engine
   - `pylang()`: Finds and caches Python external language object for expression evaluation
   - `request_install_cli()`: Asynchronously [un]installs CLIs via UI requests

3. **macros.hpp** - Data structures and default macros
   - `macro_def_t`: Macro definition structure (macro, expr, desc)
   - `DEFAULT_MACROS[]`: 25 predefined macros for cursor position, memory reads, selections, etc.

### Key Design Patterns

**CLI Hooking Mechanism**: The plugin intercepts `ui_install_cli` events and wraps each CLI's `execute_line()` function with a macro expansion layer. It maintains up to 18 concurrent CLI contexts (`MAX_CTX`) using function pointer arrays.

**Macro Expansion**: Two-phase replacement:
1. Static macros: Direct text substitution using combined regex pattern
2. Dynamic macros: Python expression evaluation for `${expression}$` patterns

**Persistence**: Macros are stored in the Windows registry (`IDAREG_CLI_MACROS`) with custom serialization using `\x1` separator.

## SDK and IDA API Resources

When answering SDK/API questions, grep and read from:
- **SDK Headers**: `$IDASDK/include` - All headers contain docstrings
- **SDK Examples**: `$IDASDK/plugins`, `$IDASDK/loaders`, `$IDASDK/module`

## Important Notes

- **utils_impl.cpp is NOT compiled separately** - It's directly included in climacros.cpp:20
- Plugin only loads in GUI mode (`is_idaq()` check)
- Debug builds use `Ctrl-Shift-A` hotkey; release builds have no hotkey (accessed via Ctrl-3)
