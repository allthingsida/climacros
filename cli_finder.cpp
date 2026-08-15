// Copyright (c) 2019-2026 Elias Bachaalany
// SPDX-License-Identifier: LicenseRef-Human-Origin-Source-1.0
//
// This file is licensed under the Human-Origin Source License v1.0.
// See LICENSE.

/*
CLI Finder: locate IDA's cli_t structures inside a loaded module's image.

Cross-platform module lookup ([base, size)) and byte-pattern search come from
libidacpp::mem. What remains here is IDA-specific: recognizing a cli_t by the
pointer it holds to its lname string.
*/

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "idasdk.h"
#include "cli_finder.h"

#include <libidacpp/mem/mem.hpp>

//-------------------------------------------------------------------------
// Given a module image and a pointer to a target string within it, find the
// cli_t that references that string through its lname field.
static const cli_t *find_cli_struct(const uint8_t *base, size_t size, const uint8_t *target_str)
{
    const uintptr_t target_addr = (uintptr_t)target_str;

    const uint8_t *ptr = base;
    const uint8_t *end = base + size - sizeof(uintptr_t);

    while (ptr <= end)
    {
        const uintptr_t potential_ptr = *(const uintptr_t *)ptr;

        // Does this slot point at our target string?
        if (potential_ptr == target_addr)
        {
            // cli_t layout: size(8), flags(4+pad), sname(8), lname(8), ...
            // so lname sits at offset 0x10 — back up to the struct start.
            const cli_t *potential_cli = (const cli_t *)(ptr - offsetof(cli_t, lname));

            // Validate: the size field must match, and sname must be non-null.
            if (potential_cli->size == sizeof(cli_t) && potential_cli->sname != nullptr)
                return potential_cli;
        }

        ++ptr;
    }

    return nullptr;
}

//-------------------------------------------------------------------------
cli_t *find_cli_in_module(const char *module_name, const char *target_string)
{
    auto range = libidacpp::mem::module_range(module_name);
    if (!range)
        return nullptr;

    const uint8_t *found_str = libidacpp::mem::find_pattern(
        range->base,
        range->size,
        (const uint8_t *)target_string,
        strlen(target_string));
    if (found_str == nullptr)
        return nullptr;

    const cli_t *cli = find_cli_struct(range->base, range->size, found_str);
    return const_cast<cli_t *>(cli);
}

//-------------------------------------------------------------------------
cli_t *find_python_cli()
{
#ifdef _WIN32
    const char *module_name = "idapython3.dll";
#elif defined(__linux__)
    const char *module_name = "idapython3.so";
#elif defined(__APPLE__)
    const char *module_name = "idapython3.dylib";
#else
    return nullptr;
#endif

    return find_cli_in_module(module_name, "Python - IDAPython plugin");
}

//-------------------------------------------------------------------------
cli_t *find_idc_cli()
{
#ifdef _WIN32
    const char *module_name = "ida.exe";
#else
    const char *module_name = "ida";
#endif

    return find_cli_in_module(module_name, "IDC - Native built-in language");
}
