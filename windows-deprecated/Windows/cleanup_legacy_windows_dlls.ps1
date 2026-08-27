param(
    [Parameter(Mandatory=$true)]
    [string]$GameSystemDir
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $GameSystemDir)) {
    throw "GameSystemDir nicht gefunden: $GameSystemDir"
}

$BackupDir = Join-Path $GameSystemDir "_legacy_win32_dll_backup"
New-Item -ItemType Directory -Force -Path $BackupDir | Out-Null

# Wichtig:
# Nur alte native DLLs verschieben.
# Window.u, Engine.u, *.int, *.ini usw. bleiben unangetastet.
$LegacyDlls = @(
    "Window.dll",
    "WinDrv.dll",
    "Editor.dll",
    "SoftDrv.dll"
)

foreach ($Dll in $LegacyDlls) {
    $Src = Join-Path $GameSystemDir $Dll
    if (Test-Path $Src) {
        $Dst = Join-Path $BackupDir $Dll
        Move-Item -Force $Src $Dst
        Write-Host "Verschoben: $Dll -> _legacy_win32_dll_backup"
    }
}

Write-Host "Legacy-DLL-Cleanup fertig."