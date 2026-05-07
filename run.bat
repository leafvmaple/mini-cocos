@echo off
setlocal
cd /d "%~dp0"

cmake -B build || exit /b 1
cmake --build build --config Debug || exit /b 1

if exist "build\Release\zocos.exe" (
  "build\Release\zocos.exe" %*
  exit /b %errorlevel%
)
if exist "build\RelWithDebInfo\zocos.exe" (
  "build\RelWithDebInfo\zocos.exe" %*
  exit /b %errorlevel%
)
if exist "build\zocos.exe" (
  "build\zocos.exe" %*
  exit /b %errorlevel%
)
if exist "build\Debug\zocos.exe" (
  "build\Debug\zocos.exe" %*
  exit /b %errorlevel%
)

echo Failed to find zocos.exe after build. Check your CMake generator and config.
exit /b 1
