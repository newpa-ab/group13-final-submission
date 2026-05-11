@echo off
setlocal

set "CUBE_CMAKE=%LOCALAPPDATA%\stm32cube\bundles\cmake\4.0.1+st.3\bin\cmake.exe"

if not exist "%CUBE_CMAKE%" (
    echo STM32Cube CMake not found at "%CUBE_CMAKE%".
    exit /b 1
)

"%CUBE_CMAKE%" %*
