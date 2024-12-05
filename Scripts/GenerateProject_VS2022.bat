@echo off

echo Cleaning previously generated files...

set SOLUTION_DIR=..\Solutions\DestanVS2022

if exist %SOLUTION_DIR% (
    echo Removing old solution directory...
    rmdir /s /q %SOLUTION_DIR%
)

echo Creating solution directory...
mkdir %SOLUTION_DIR%
cd %SOLUTION_DIR%

echo Generating Visual Studio 2022 solution...
cmake ..\.. -G "Visual Studio 17 2022" -A x64

if errorlevel 1 (
    echo Solution generation failed!
    pause
    exit /b 1
)

echo Solution generated successfully!
pause