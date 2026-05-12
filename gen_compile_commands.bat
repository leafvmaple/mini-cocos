@echo off
setlocal
cd /d "%~dp0"

set "NINJA_PATH="

where ninja >nul 2>nul
if not errorlevel 1 set "NINJA_PATH=ninja"

if not defined NINJA_PATH (
  for %%P in (
    "%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
    "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
    "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
    "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
  ) do (
    if not defined NINJA_PATH if exist "%%~P" set "NINJA_PATH=%%~P"
  )
)

if not defined NINJA_PATH (
  echo Ninja was not found in PATH, and no Visual Studio bundled Ninja was detected.
  echo Install Ninja, then rerun this script.
  echo Example: winget install Ninja-build.Ninja
  exit /b 1
)

set "VSDEVCMD_PATH="
set "VSWHERE_EXE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if exist "%VSWHERE_EXE%" (
  for /f "usebackq delims=" %%I in (`"%VSWHERE_EXE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    if not defined VSDEVCMD_PATH if exist "%%~I\Common7\Tools\VsDevCmd.bat" set "VSDEVCMD_PATH=%%~I\Common7\Tools\VsDevCmd.bat"
  )
)

if not defined VSDEVCMD_PATH (
  for %%P in (
    "%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
    "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat"
    "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat"
    "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Enterprise\Common7\Tools\VsDevCmd.bat"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\BuildTools\Common7\Tools\VsDevCmd.bat"
  ) do (
    if not defined VSDEVCMD_PATH if exist "%%~P" set "VSDEVCMD_PATH=%%~P"
  )
)

if defined VSDEVCMD_PATH (
  echo Initializing MSVC environment...
  call "%VSDEVCMD_PATH%" -arch=x64 -host_arch=x64 >nul
)

where cl >nul 2>nul
if errorlevel 1 (
  echo Could not find cl.exe even after VS environment initialization.
  echo Please install Visual Studio C++ build tools, or run this script in Developer PowerShell for VS.
  exit /b 1
)

echo Using Ninja: "%NINJA_PATH%"

cmake -S . -B build-clangd -G Ninja "-DCMAKE_MAKE_PROGRAM=%NINJA_PATH%" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON || exit /b 1

if not exist "build-clangd\compile_commands.json" (
  echo Failed to generate build-clangd\compile_commands.json.
  exit /b 1
)

copy /y "build-clangd\compile_commands.json" "compile_commands.json" >nul
echo Generated compile_commands.json for clangd.
exit /b 0
