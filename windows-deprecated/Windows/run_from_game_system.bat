@echo off
setlocal
if "%~1"=="" (
  echo Nutzung: run_from_game_system.bat C:\Games\Unreal\System
  echo Der Ordner muss die Unreal-Spieldaten enthalten. Spieldaten werden nicht mitgeliefert.
  exit /b 1
)
cd /d "%~1"
if not exist Unreal.exe (
  echo Unreal.exe wurde nicht gefunden. Bitte zuerst install_to_game_system.ps1 ausfuehren.
  exit /b 1
)
Unreal.exe