@echo off
setlocal
cd /d "%~dp0"

set "CKZ_MSBUILD="
set "CKZ_VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if defined CKZ_VS if exist "%CKZ_VS%\MSBuild\Current\Bin\amd64\MSBuild.exe" set "CKZ_MSBUILD=%CKZ_VS%\MSBuild\Current\Bin\amd64\MSBuild.exe"
if not defined CKZ_MSBUILD if exist "%CKZ_VSWHERE%" (
    for /f "usebackq delims=" %%V in (`call "%CKZ_VSWHERE%" -latest -products * -version "[17.0,19.0)" -requires Microsoft.Component.MSBuild Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find MSBuild\Current\Bin\amd64\MSBuild.exe`) do set "CKZ_MSBUILD=%%V"
)
if not defined CKZ_MSBUILD (
    echo Install Visual Studio 2022 or 2026 with the Windows C++ x86 tools and Windows SDK.
    exit /b 1
)
echo Using MSBuild: "%CKZ_MSBUILD%"
"%CKZ_MSBUILD%" "%CD%\vs\ckz_pc.sln" /m /p:Configuration=Release /p:Platform=Win32
if errorlevel 1 exit /b 1
echo Game: %CD%\bin\Win32\Release\ckz_pc.exe
