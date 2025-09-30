@echo off

:: checkout the Batchography book

setlocal

if not defined IDASDK (
    echo IDASDK environment variable not set.
    echo Also make sure ida-cmake is installed in IDASDK.
    echo See: https://github.com/allthingsida/ida-cmake
    goto :eof
)

if not exist %IDASDK%\include\idax\xkernwin.hpp (
    echo IDAX framework not properly installed in the IDA SDK folder.
    echo See: https://github.com/allthingsida/idax
    goto :eof
)

if not exist build cmake -A x64 -B build

if "%1"=="build" cmake --build build --config Release

echo.
echo All done!
echo.