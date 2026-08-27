@echo off
setlocal
set "UE1ROOT=%~dp0"
set "UE1SRC=%UE1ROOT%Source"
set "UE1CMAKE=%UE1ROOT%Builds\_cmake\vs2026-win32-sdl-opengl"
set "UE1OUT=%UE1ROOT%Builds\RelWithDebInfo"

echo [UE1 v23] Configure VS2026 Win32 SDL/OpenGL...
cmake --preset vs2026-win32-sdl-opengl -S "%UE1SRC%" -DBUILD_NOPENGLDRV=ON -DBUILD_NOPENGLESDRV=ON
if errorlevel 1 exit /b %errorlevel%

echo [UE1 v23] Build ALL_BUILD / RelWithDebInfo / Win32...
cmake --build "%UE1CMAKE%" --config RelWithDebInfo --target ALL_BUILD -- /m:1
if errorlevel 1 exit /b %errorlevel%

echo.
echo [UE1 v23] Ausgabe:
echo   %UE1OUT%
echo.
if exist "%UE1OUT%" dir "%UE1OUT%"
endlocal