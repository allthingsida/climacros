cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(climacros)

include($ENV{IDASDK}/ida-cmake/common.cmake)

# Included file
list(APPEND DISABLED_SOURCES utils_impl.cpp)

set(PLUGIN_NAME              climacros)
set(PLUGIN_SOURCES           climacros.cpp macros.hpp idasdk.h ${DISABLED_SOURCES})
set(PLUGIN_RUN_ARGS          "-t") # Debug messages for the debugger
set(PLUGIN_OUTPUT_NAME       "00${PROJECT_NAME}")

set_source_files_properties(${DISABLED_SOURCES} PROPERTIES LANGUAGE "")

generate()