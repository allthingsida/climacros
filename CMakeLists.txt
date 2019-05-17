cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

# Included file
list(APPEND DISABLED_SOURCES utils_impl.cpp)

set(PLUGIN_NAME              climacros)
set(PLUGIN_SOURCES           climacros.cpp ${DISABLED_SOURCES})

include($ENV{IDASDK}/ida-cmake/plugins.cmake)

set_source_files_properties(${DISABLED_SOURCES} PROPERTIES LANGUAGE "")