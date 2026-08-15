// Copyright (c) 2019-2026 Elias Bachaalany
// SPDX-License-Identifier: LicenseRef-Human-Origin-Source-1.0
//
// This file is licensed under the Human-Origin Source License v1.0.
// See LICENSE.

#pragma once

#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4267 4244 4146)
#endif
#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <kernwin.hpp>
#include <expr.hpp>
#include <registry.hpp>
#include <diskio.hpp>
#ifdef _MSC_VER
    #pragma warning(pop)
#endif

// Undefine IDA SDK macros that conflict with standard library member functions
#ifdef wait
    #undef wait
#endif
#ifdef waitpid
    #undef waitpid
#endif
#ifdef waitid
    #undef waitid
#endif
#ifdef strupr
    #undef strupr
#endif
#ifdef strlwr
    #undef strlwr
#endif

#include <libidacpp/expr/expr.hpp>

using libidacpp::expr::pylang;
