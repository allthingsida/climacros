// Copyright (c) 2019-2026 Elias Bachaalany
// SPDX-License-Identifier: LicenseRef-Human-Origin-Source-1.0
//
// This file is licensed under the Human-Origin Source License v1.0.
// See LICENSE.

/*
CLI Hooking: wrap a cli_t's execute_line() so CLI input is run through the macro
replacer before IDA sees it, plus the async (un)install request helper.
*/

#pragma once

struct cli_t;

// Max number of concurrently hooked CLIs (fixed-size callback registry).
constexpr int MAX_CLIS = 20;

// Hook a CLI's execute_line to enable macro expansion.
// Returns the hooked CLI copy, or nullptr if hooking failed (no free slots).
const cli_t *hook_cli(const cli_t *cli);

// Unhook a previously hooked CLI. Returns the hooked copy, or nullptr if not found.
const cli_t *unhook_cli(const cli_t *cli);

// Ignore UI hook notifications while we are the ones (un)installing a CLI.
extern bool g_b_ignore_ui_notification;

// [Un]install a CLI (asynchronously) via a UI request.
void request_install_cli(const cli_t *cli, bool install);
