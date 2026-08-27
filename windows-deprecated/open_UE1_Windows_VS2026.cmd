@echo off
setlocal
set "UE1ROOT=%~dp0"
set "UE1SLNX=%UE1ROOT%UE1_Windows_VS2026_With_NOpenGLESDrv.slnx"

if not exist "%UE1SLNX%" (
  set "UE1SLNX=%UE1ROOT%UE1_Windows_VS2026.slnx"
)

if not exist "%UE1SLNX%" (
  echo [UE1] Root-slnx fehlt. Bitte zuerst CMake konfigurieren/builden.
  exit /b 1
)

start "" "%UE1SLNX%"
endlocal