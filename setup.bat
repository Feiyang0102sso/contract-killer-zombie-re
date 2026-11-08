@echo off
rem One-time setup: create the ckz-build WSL distro in .wsl\ and install the toolchain.
rem Usage: setup.bat [path to installxcode_421_lion.dmg]
setlocal
if "%~1"=="" (
    powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0toolchain\setup_wsl.ps1"
) else (
    powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0toolchain\setup_wsl.ps1" -XcodeDmg "%~f1"
)
set STATUS=%ERRORLEVEL%
if not "%STATUS%"=="0" echo setup failed with exit code %STATUS%
pause
exit /b %STATUS%
