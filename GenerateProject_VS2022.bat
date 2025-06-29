@echo off
setlocal enabledelayedexpansion

echo DS Engine - Visual Studio 2022 Project Generator

:: Output directory
set CURRENT_DIR=%CD%

echo Generating Visual Studio 2022 solution...
Scripts\premake5.exe vs2022

if errorlevel 1 (
    echo Solution generation failed!
    pause
    exit /b 1
)

echo Solution successfully generated at: %CURRENT_DIR%
echo Opening solution...

start "" "%CURRENT_DIR%\Ds.sln"
pause