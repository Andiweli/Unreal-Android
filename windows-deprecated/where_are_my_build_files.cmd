@echo off
setlocal
set "UE1ROOT=%~dp0"
echo [UE1 v23] Nutzbare Build-Ausgaben:
echo.
for %%C in (Debug Release RelWithDebInfo MinSizeRel) do (
  echo --- Builds\%%C ---
  if exist "%UE1ROOT%Builds\%%C" (
    dir "%UE1ROOT%Builds\%%C"
  ) else (
    echo fehlt noch
  )
  echo.
)
echo [UE1 v23] CMake/VS-Arbeitsordner:
echo   %UE1ROOT%Builds\_cmake\vs2026-win32-sdl-opengl
if exist "%UE1ROOT%Builds\_cmake\vs2026-win32-sdl-opengl" (echo vorhanden) else (echo fehlt noch)
endlocal