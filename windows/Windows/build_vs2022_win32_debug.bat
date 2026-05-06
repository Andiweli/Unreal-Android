@echo off
setlocal
cd /d "%~dp0..\Source"
where cmake >nul 2>nul
if errorlevel 1 (
  echo CMake wurde nicht gefunden. Bitte Visual Studio 2022 mit C++/CMake-Tools installieren oder das VS2026-Skript verwenden.
  exit /b 1
)
cmake --preset vs2022-win32-sdl-opengl
if errorlevel 1 exit /b %errorlevel%
cmake --build --preset build-vs2022-win32-debug
exit /b %errorlevel%