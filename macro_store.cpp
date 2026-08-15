// Copyright (c) 2019-2026 Elias Bachaalany
// SPDX-License-Identifier: LicenseRef-Human-Origin-Source-1.0
//
// This file is licensed under the Human-Origin Source License v1.0.
// See LICENSE.

/*
Macro Store implementation. PURE (no IDA SDK) so it can be unit-tested headlessly.
*/

#include "macro_store.h"

#include <algorithm>

//-------------------------------------------------------------------------
macro_def_t macro_def_t::parse(const std::string &ser)
{
    const char sep = SER_SEPARATOR[0];
    macro_def_t def;
    std::string *fields[3] = { &def.macro, &def.expr, &def.desc };

    size_t start = 0;
    for (int i = 0; i < 3; ++i)
    {
        if (start > ser.size())
            break;
        size_t pos = (i < 2) ? ser.find(sep, start) : std::string::npos;
        *fields[i] = ser.substr(start, pos == std::string::npos ? std::string::npos : pos - start);
        if (pos == std::string::npos)
            break;
        start = pos + 1;
    }
    return def;
}

//-------------------------------------------------------------------------
void macro_store_t::load(const macro_def_t *defaults, size_t count)
{
    std::vector<std::string> ser;
    m_backend.read_all(ser);
    m_macros.clear();

    if (ser.empty())
    {
        // Nothing stored. Populate defaults once, guarded by the first-run marker.
        if (!m_backend.first_run_done())
        {
            for (size_t i = 0; i < count; ++i)
            {
                m_macros.push_back(defaults[i]);
                m_backend.add(defaults[i].to_string());
            }
            // Mark ONLY after every default is persisted, so a crash mid-populate
            // cannot leave us permanently empty (marker set, registry empty).
            m_backend.mark_first_run_done();
        }
        return;
    }

    for (const std::string &s : ser)
        m_macros.push_back(macro_def_t::parse(s));
}

//-------------------------------------------------------------------------
bool macro_store_t::add(const macro_def_t &def)
{
    if (find(def.macro) != nullptr)
        return false;   // duplicate trigger name

    m_backend.add(def.to_string());
    m_macros.push_back(def);
    return true;
}

//-------------------------------------------------------------------------
bool macro_store_t::remove(const macro_def_t &def)
{
    auto it = std::find_if(m_macros.begin(), m_macros.end(),
                           [&](const macro_def_t &m) { return m.macro == def.macro; });
    if (it == m_macros.end())
        return false;

    // Delete the exact serialized entry that was stored for this macro.
    m_backend.remove(it->to_string());
    m_macros.erase(it);
    return true;
}

//-------------------------------------------------------------------------
bool macro_store_t::update(const macro_def_t &old_def, const macro_def_t &new_def)
{
    auto it = std::find_if(m_macros.begin(), m_macros.end(),
                           [&](const macro_def_t &m) { return m.macro == old_def.macro; });
    if (it == m_macros.end())
        return false;

    // A rename must not collide with a different existing macro.
    if (new_def.macro != old_def.macro && find(new_def.macro) != nullptr)
        return false;

    m_backend.remove(it->to_string());
    m_backend.add(new_def.to_string());
    *it = new_def;
    return true;
}

//-------------------------------------------------------------------------
const macro_def_t *macro_store_t::find(const std::string &name) const
{
    for (const macro_def_t &m : m_macros)
        if (m.macro == name)
            return &m;
    return nullptr;
}
