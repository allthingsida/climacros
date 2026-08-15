# Copyright (c) 2019-2026 Elias Bachaalany
# SPDX-License-Identifier: LicenseRef-Human-Origin-Source-1.0
#
# This file is licensed under the Human-Origin Source License v1.0.
# See LICENSE.

cmake_minimum_required(VERSION 3.27)
project(climacros)
set(CMAKE_CXX_STANDARD 20)

# Include ida-cmake bootstrap (installed alongside the SDK)
include($ENV{IDASDK}/ida-cmake/bootstrap.cmake)
find_package(idasdk REQUIRED)

# Resolve libidacpp: prefer the local monorepo sibling (../libidacpp) for
# development; fall back to FetchContent for CI / out-of-tree builds.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../libidacpp/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../libidacpp ${CMAKE_CURRENT_BINARY_DIR}/libidacpp)
else()
    include(FetchContent)
    FetchContent_Declare(
        libidacpp
        GIT_REPOSITORY https://github.com/allthingsida/libidacpp.git
        GIT_TAG main
    )
    FetchContent_MakeAvailable(libidacpp)
endif()

# Add plugin using ida-cmake helper function
ida_add_plugin(climacros
    SOURCES
        plugin.cpp
        cli_finder.cpp
        cli_finder.h
        cli_hook.cpp
        cli_hook.h
        extlang_utils.cpp
        extlang_utils.h
        macro_replacer.cpp
        macro_replacer.h
        macro_store.cpp
        macro_store.h
        macro_store_registry.cpp
        macro_store_registry.h
        macro_editor.cpp
        macro_editor.h
        idasdk.h
        README.md
    OUTPUT_NAME
        climacros
    METADATA_JSON
        ida-plugin.json
    DEBUG_ARGS
        "-t"
)

# Link libidacpp
target_link_libraries(climacros PRIVATE libidacpp::libidacpp)
