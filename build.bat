@echo off
rem One-click build: compile + link in the ckz-build WSL distro; result in _output\zombsniper.
rem Usage: build.bat [-B]   (-B recompiles every object)
setlocal
set DISTRO=ckz-build
wsl.exe -d %DISTRO% -u root --cd "%~dp0." --exec bash toolchain/build.sh %*
set STATUS=%ERRORLEVEL%
if "%STATUS%"=="0" (
    echo build finished: %~dp0_output\zombsniper
) else (
    echo build failed with exit code %STATUS%
)
pause
exit /b %STATUS%
