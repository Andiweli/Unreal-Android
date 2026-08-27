@echo off
setlocal
cd /d "%~dp0..\Source"
where cmake >nul 2>nul
if errorlevel 1 (
  echo CMake wurde nicht gefunden. Bitte Visual Studio 2026 mit C++/CMake-Tools oder CMake 4.2+ installieren.
  exit /b 1
)
cmake --preset vs2026-win32-sdl-opengl
if errorlevel 1 exit /b %errorlevel%
cmake --build --preset build-vs2026-win32-relwithdebinfo
exit /b %errorlevel%