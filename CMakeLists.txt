cmake_minimum_required(VERSION 3.27)
project(climacros)
set(CMAKE_CXX_STANDARD 20)

# Include IDA SDK bootstrap
include($ENV{IDASDK}/ida-cmake/bootstrap.cmake)
find_package(idasdk REQUIRED)

# Add idacpp library (disable examples to avoid build issues)
set(IDACPP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory($ENV{IDASDK}/include/idacpp ${CMAKE_CURRENT_BINARY_DIR}/idacpp)

# Add plugin using ida-cmake helper function
ida_add_plugin(climacros
    SOURCES
        plugin.cpp
        cli_utils.cpp
        cli_utils.h
        macro_editor.cpp
        macro_editor.h
        idasdk.h
        README.md
    OUTPUT_NAME
        climacros
    DEBUG_ARGS
        "-t"
)

# Link idacpp
target_link_libraries(climacros PRIVATE idacpp::idacpp)