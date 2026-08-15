// Copyright (c) 2019-2026 Elias Bachaalany
// SPDX-License-Identifier: LicenseRef-Human-Origin-Source-1.0
//
// This file is licensed under the Human-Origin Source License v1.0.
// See LICENSE.

#include "idasdk.h"
#include "macro_store_registry.h"
#include <libidacpp/storage/registry.hpp>

//-------------------------------------------------------------------------
// The macro list, persisted as an IDA registry strlist under IDAREG_CLI_MACROS.
static libidacpp::storage::registry::strlist_t &cli_macros()
{
    static libidacpp::storage::registry::strlist_t list(IDAREG_CLI_MACROS, MAX_CLI_MACROS);
    return list;
}

//-------------------------------------------------------------------------
static void first_run_marker_path(qstring &out)
{
    out.sprnt("%s/firstrun.climacros", get_user_idadir());
}

//-------------------------------------------------------------------------
void registry_backend_t::read_all(std::vector<std::string> &out)
{
    out = cli_macros().read();
}

//-------------------------------------------------------------------------
void registry_backend_t::add(const std::string &ser)
{
    cli_macros().add(ser);
}

//-------------------------------------------------------------------------
void registry_backend_t::remove(const std::string &ser)
{
    cli_macros().remove(ser);
}

//-------------------------------------------------------------------------
bool registry_backend_t::first_run_done()
{
    qstring path;
    first_run_marker_path(path);
    return qfileexist(path.c_str());
}

//-------------------------------------------------------------------------
void registry_backend_t::mark_first_run_done()
{
    qstring path;
    first_run_marker_path(path);
    FILE *fp = qfopen(path.c_str(), "w");
    if (fp != nullptr)
        qfclose(fp);
}
