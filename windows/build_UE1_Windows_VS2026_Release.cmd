@echo off
setlocal
cd /d "%~dp0"
echo [UE1 v24] Configure VS2026 Win32 SDL/OpenGL...
cmake -S "Source" -B "Builds\_cmake\vs2026-win32-sdl-opengl" -G "Visual Studio 18 2026" -A Win32 -DUSE_SDL=ON -DBUILD_NOPENGLDRV=ON -DBUILD_NOPENGLESDRV=OFF -DBUILD_EDITOR=OFF -DBUILD_WINDRV=OFF -DBUILD_SOFTDRV=OFF -DBUILD_NOPENALDRV=ON -DBUILD_NULLSOUNDDRV=OFF
if errorlevel 1 exit /b %errorlevel%
echo [UE1 v24] Build ALL_BUILD / Release / Win32...
cmake --build "Builds\_cmake\vs2026-win32-sdl-opengl" --config Release --target ALL_BUILD -- /m:1
exit /b %errorlevel%