// Copyright (c) 2019-2026 Elias Bachaalany
// SPDX-License-Identifier: LicenseRef-Human-Origin-Source-1.0
//
// This file is licensed under the Human-Origin Source License v1.0.
// See LICENSE.

/*
CLI Finder: locate IDA's cli_t structures by scanning a loaded module's image.
The cross-platform "where is this module in memory" and byte-pattern search live
in libidacpp::mem; this header only exposes the cli_t-specific lookups.
*/

#pragma once

struct cli_t;

// Find a cli_t structure in a loaded module by searching for a target string in
// its lname field.
//   module_name   - module to search (e.g. "idapython3.dll", "ida.exe")
//   target_string - string to match in the CLI's lname field
// Returns the cli_t if found, nullptr otherwise.
cli_t *find_cli_in_module(const char *module_name, const char *target_string);

// Helper: find the Python CLI ("Python - IDAPython plugin") in idapython3.*
cli_t *find_python_cli();

// Helper: find the IDC CLI ("IDC - Native built-in language") in the IDA module
cli_t *find_idc_cli();
