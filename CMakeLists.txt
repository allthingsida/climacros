cmake_minimum_required(VERSION 3.27)
project(climacros)
set(CMAKE_CXX_STANDARD 20)

# Include IDA SDK bootstrap
include($ENV{IDASDK}/ida-cmake/bootstrap.cmake)
find_package(idasdk REQUIRED)

# Add plugin using ida-cmake helper function
ida_add_plugin(climacros
    SOURCES
        climacros.cpp
        cli_utils.cpp
        cli_utils.h
        macro_editor.cpp
        macro_editor.h
        idasdk.h
    OUTPUT_NAME
        climacros
    DEBUG_ARGS
        "-t"
)