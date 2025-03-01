@echo off
setlocal enabledelayedexpansion

echo Cleaning previously generated files...

set SOLUTION_DIR=..\Solutions\DestanVS2022

if exist %SOLUTION_DIR% (
    echo Removing old solution directory...
    rd /s /q %SOLUTION_DIR% 2>nul
    timeout /t 2 >nul
    if exist %SOLUTION_DIR% (
        echo Error: Could not remove old solution directory.
        echo Please close Visual Studio and any applications using the files.
        pause
        exit /b 1
    )
)

echo Creating solution directory...
if not exist ..\Solutions mkdir ..\Solutions
if not exist %SOLUTION_DIR% mkdir %SOLUTION_DIR% 2>nul
if errorlevel 1 (
    echo Error: Could not create solution directory.
    pause
    exit /b 1
)

cd %SOLUTION_DIR%
if errorlevel 1 (
    echo Error: Could not enter solution directory.
    pause
    exit /b 1
)

echo Generating Visual Studio 2022 solution...
cmake ..\.. -G "Visual Studio 17 2022" -A x64

if errorlevel 1 (
    echo Solution generation failed!
    pause
    exit /b 1
)

echo Solution generated successfully!
pause