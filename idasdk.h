#pragma once

#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4267 4244 4146)
#endif
#include <ida.hpp>
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

#include <idax/xexpr.hpp>
