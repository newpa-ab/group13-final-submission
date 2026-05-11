@echo off
setlocal

set "CUBE_ROOT=%LOCALAPPDATA%\stm32cube\bundles"

if "%~1"=="" (
    echo Usage: cube ^<tool^> [args...]
    exit /b 1
)

set "TOOL=%~1"
shift

if /I "%TOOL%"=="starm-clangd" (
    set "TOOL_PATH=%CUBE_ROOT%\clangd\starm-clangd.exe"
) else (
    echo Unsupported STM32Cube tool "%TOOL%".
    exit /b 1
)

if not exist "%TOOL_PATH%" (
    echo STM32Cube tool not found at "%TOOL_PATH%".
    exit /b 1
)

"%TOOL_PATH%" %*
