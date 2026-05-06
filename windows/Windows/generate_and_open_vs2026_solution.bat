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
echo.
echo Visual-Studio-Solution:
echo %CD%\build\vs2026-win32-sdl-opengl\UE1.sln
echo.
if exist "%CD%\build\vs2026-win32-sdl-opengl\UE1.sln" start "" "%CD%\build\vs2026-win32-sdl-opengl\UE1.sln"
exit /b 0