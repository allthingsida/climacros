// Copyright (c) 2019-2026 Elias Bachaalany
// SPDX-License-Identifier: LicenseRef-Human-Origin-Source-1.0
//
// This file is licensed under the Human-Origin Source License v1.0.
// See LICENSE.

/*
Macro Store: persistence-layer for CLI macros, decoupled from the editor UI.

This header is PURE (no IDA SDK) so the store, serialization and default/first-run
logic can be unit-tested headlessly with an in-memory backend. The concrete
IDA-registry backend lives in macro_store_registry.{h,cpp}.
*/

#pragma once

#include <string>
#include <vector>

//-------------------------------------------------------------------------
// Constants for macro serialization and registry storage
//-------------------------------------------------------------------------
constexpr char IDAREG_CLI_MACROS[] = "CLI_Macros";
constexpr int  MAX_CLI_MACROS      = 200;
constexpr char SER_SEPARATOR[]     = "\x1";

//-------------------------------------------------------------------------
// Macro definition + (de)serialization
//-------------------------------------------------------------------------
struct macro_def_t
{
    std::string macro;
    std::string expr;
    std::string desc;

    // Identity is the macro trigger name (used for de-dup / lookup).
    bool operator==(const macro_def_t &rhs) const { return macro == rhs.macro; }

    // Serialize the three fields joined by SER_SEPARATOR.
    void to_string(std::string &str) const
    {
        str = macro + SER_SEPARATOR + expr + SER_SEPARATOR + desc;
    }
    std::string to_string() const
    {
        std::string s;
        to_string(s);
        return s;
    }

    // Parse a serialized entry. Splits on SER_SEPARATOR keeping empty fields, so
    // an empty expression/description round-trips correctly (fields 0/1/2 map to
    // macro/expr/desc; missing trailing fields become empty).
    static macro_def_t parse(const std::string &ser);
};

//-------------------------------------------------------------------------
// Storage backend interface (injected -> testable)
//-------------------------------------------------------------------------
struct imacro_store_backend_t
{
    virtual ~imacro_store_backend_t() = default;

    // Read every persisted (serialized) macro entry.
    virtual void read_all(std::vector<std::string> &out) = 0;
    // Add / remove a single serialized entry.
    virtual void add(const std::string &ser) = 0;
    virtual void remove(const std::string &ser) = 0;
    // First-run marker: have the defaults already been populated once?
    virtual bool first_run_done() = 0;
    virtual void mark_first_run_done() = 0;
};

//-------------------------------------------------------------------------
// Macro store: the in-memory list kept in sync with a backend.
//-------------------------------------------------------------------------
class macro_store_t
{
public:
    explicit macro_store_t(imacro_store_backend_t &backend) : m_backend(backend) {}

    // Load the persisted macros. If none are stored AND the first-run marker is
    // absent, populate `defaults` (persisting each BEFORE marking first-run done,
    // so a crash mid-populate can never leave us permanently empty).
    void load(const macro_def_t *defaults, size_t count);

    // Mutations. add()/update() return false if the (new) name already exists.
    bool add(const macro_def_t &def);
    bool remove(const macro_def_t &def);
    bool update(const macro_def_t &old_def, const macro_def_t &new_def);

    const macro_def_t *find(const std::string &name) const;
    const std::vector<macro_def_t> &list() const { return m_macros; }

private:
    imacro_store_backend_t   &m_backend;
    std::vector<macro_def_t>  m_macros;
};
