param(
    [string]$Root = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

function Backup-Once($Path) {
    $Backup = "$Path.bak_nopenglesdrv_vs2026"
    if (-not (Test-Path $Backup)) {
        Copy-Item $Path $Backup -Force
        Write-Host "Backup erstellt: $Backup"
    }
}

function Read-Text($Path) {
    [System.IO.File]::ReadAllText($Path)
}

function Write-Text($Path, $Text) {
    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $Text, $utf8NoBom)
}

$Presets = Join-Path $Root "Source\CMakePresets.json"
$BuildRelDbg = Join-Path $Root "build_UE1_Windows_VS2026_RelWithDebInfo.cmd"
$BuildRel = Join-Path $Root "build_UE1_Windows_VS2026_Release.cmd"
$OpenScript = Join-Path $Root "open_UE1_Windows_VS2026.cmd"

if (-not (Test-Path $Presets)) {
    throw "Nicht gefunden: $Presets"
}

if (-not (Test-Path (Join-Path $Root "Source\NOpenGLESDrv\CMakeLists.txt"))) {
    throw "NOpenGLESDrv-Source fehlt: Source\NOpenGLESDrv\CMakeLists.txt"
}

Backup-Once $Presets

# CMakePresets.json: im VS2026 OpenGL-Preset NOpenGLESDrv einschalten.
$Json = Read-Text $Presets

$Pattern = '("name"\s*:\s*"vs2026-win32-sdl-opengl"[\s\S]*?"BUILD_NOPENGLESDRV"\s*:\s*)"OFF"'
if ($Json -match $Pattern) {
    $Json = [regex]::Replace($Json, $Pattern, '$1"ON"', 1)
    Write-Text $Presets $Json
    Write-Host "Gepatcht: Source\CMakePresets.json -> BUILD_NOPENGLESDRV ON"
}
elseif ($Json -match '"name"\s*:\s*"vs2026-win32-sdl-opengl"[\s\S]*?"BUILD_NOPENGLESDRV"\s*:\s*"ON"') {
    Write-Host "Bereits korrekt: BUILD_NOPENGLESDRV ist ON"
}
else {
    throw "Konnte BUILD_NOPENGLESDRV im Preset vs2026-win32-sdl-opengl nicht finden."
}

# Build-Skripte zusätzlich absichern: Command-Line Override für CMake.
foreach ($Script in @($BuildRelDbg, $BuildRel)) {
    if (Test-Path $Script) {
        Backup-Once $Script
        $Txt = Read-Text $Script

        if ($Txt -notmatch 'BUILD_NOPENGLESDRV=ON') {
            $Txt = $Txt -replace 'cmake --preset vs2026-win32-sdl-opengl -S "%UE1SRC%"',
                                'cmake --preset vs2026-win32-sdl-opengl -S "%UE1SRC%" -DBUILD_NOPENGLDRV=ON -DBUILD_NOPENGLESDRV=ON'
            Write-Text $Script $Txt
            Write-Host "Gepatcht: $([System.IO.Path]::GetFileName($Script))"
        }
        else {
            Write-Host "Bereits gepatcht: $([System.IO.Path]::GetFileName($Script))"
        }
    }
}

# Open-Script auf deine With_NOpenGLESDrv.slnx ausrichten.
if (Test-Path $OpenScript) {
    Backup-Once $OpenScript

    $OpenTxt = @'
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
'@

    Write-Text $OpenScript $OpenTxt
    Write-Host "Gepatcht: open_UE1_Windows_VS2026.cmd"
}

Write-Host ""
Write-Host "Patch fertig. Jetzt bitte CMake-Buildordner neu erzeugen."