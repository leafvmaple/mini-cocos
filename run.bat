@echo off
setlocal
cd /d "%~dp0"

set "BUILD_DIR=build-run"
set "BUILD_CONFIG=Debug"
set "PYTHONUTF8=1"
set "PYTHONIOENCODING=utf-8"

cmake -S . -B "%BUILD_DIR%" || exit /b 1
cmake --build "%BUILD_DIR%" --config %BUILD_CONFIG% || exit /b 1

if exist "%BUILD_DIR%\Debug\zocos.exe" (
  "%BUILD_DIR%\Debug\zocos.exe" %*
  exit /b %errorlevel%
)
if exist "%BUILD_DIR%\Release\zocos.exe" (
  "%BUILD_DIR%\Release\zocos.exe" %*
  exit /b %errorlevel%
)
if exist "%BUILD_DIR%\RelWithDebInfo\zocos.exe" (
  "%BUILD_DIR%\RelWithDebInfo\zocos.exe" %*
  exit /b %errorlevel%
)
if exist "%BUILD_DIR%\zocos.exe" (
  "%BUILD_DIR%\zocos.exe" %*
  exit /b %errorlevel%
)

echo Failed to find zocos.exe after build. Check your CMake generator and config.
exit /b 1
