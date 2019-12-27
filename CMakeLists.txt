cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(climacros)

# Included file
list(APPEND DISABLED_SOURCES utils_impl.cpp)

set(PLUGIN_NAME              climacros)
set(PLUGIN_SOURCES           climacros.cpp ${DISABLED_SOURCES})
set(PLUGIN_RUN_ARGS          "-t") # Debug messages for the debugger

include($ENV{IDASDK}/ida-cmake/plugins.cmake)

set_source_files_properties(${DISABLED_SOURCES} PROPERTIES LANGUAGE "")