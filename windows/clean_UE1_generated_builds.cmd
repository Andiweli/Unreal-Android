@echo off
setlocal
set "UE1ROOT=%~dp0"
echo [UE1 v23] Entferne generierte Build-/VS-Dateien...
if exist "%UE1ROOT%.vs" rmdir /s /q "%UE1ROOT%.vs"
if exist "%UE1ROOT%Source\build" rmdir /s /q "%UE1ROOT%Source\build"
if exist "%UE1ROOT%BUILDS" rmdir /s /q "%UE1ROOT%BUILDS"
if exist "%UE1ROOT%Builds" rmdir /s /q "%UE1ROOT%Builds"
echo [UE1 v23] Fertig. Danach build_UE1_Windows_VS2026_RelWithDebInfo.cmd neu starten.
endlocal