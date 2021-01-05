@echo off

:: checkout the Batchography book

setlocal

if not defined IDASDK (
    echo IDASDK environment variable not set.
    echo Also make sure ida-cmake is installed in IDASDK.
    echo See: https://github.com/0xeb/ida-cmake
    goto :eof
)

if not exist build (
    mkdir build
    pushd build
    cmake -A x64 -G "Visual Studio 16 2019" ..
    popd
)

if not exist build64 (
    mkdir build64
    pushd build64
    cmake -A x64 -DEA64=YES -G "Visual Studio 16 2019" ..
    popd
)

echo.
echo All done!
echo.