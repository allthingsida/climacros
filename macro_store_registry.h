// Copyright (c) 2019-2026 Elias Bachaalany
// SPDX-License-Identifier: LicenseRef-Human-Origin-Source-1.0
//
// This file is licensed under the Human-Origin Source License v1.0.
// See LICENSE.

/*
Registry-backed macro store backend (production). Persists macros in the IDA
registry list HKCU\...\CLI_Macros and tracks the first-run marker file. This is
the IDA-dependent counterpart to the pure macro_store.{h,cpp}.
*/

#pragma once

#include "macro_store.h"

// Persists macros to the IDA registry (reg_*_strlist) and the first-run marker
// file under get_user_idadir().
struct registry_backend_t : public imacro_store_backend_t
{
    void read_all(std::vector<std::string> &out) override;
    void add(const std::string &ser) override;
    void remove(const std::string &ser) override;
    bool first_run_done() override;
    void mark_first_run_done() override;
};
