param(
    [Parameter(Mandatory=$true)][string]$GameSystemDir,
    [ValidateSet("Debug","Release","RelWithDebInfo","MinSizeRel")][string]$Config = "RelWithDebInfo",
    [string]$PresetBuildDir = "vs2026-win32-sdl-opengl"
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$Ue1Root = Split-Path -Parent $ScriptDir
$Bin = Join-Path $Ue1Root "Builds\\_cmake\\\$PresetBuildDir\bin\$Config"
if(!(Test-Path $Bin)) {
    throw "Build-Ausgabe nicht gefunden: $Bin. Vorher windows\build_vs2026_win32_relwithdebinfo.bat ausfÃƒÆ’Ã‚Â¼hren."
}
if(!(Test-Path $GameSystemDir)) {
    throw "GameSystemDir existiert nicht: $GameSystemDir. Gemeint ist der System-Ordner deiner legalen Unreal-Installation, z.B. C:\Games\Unreal\System."
}

Get-ChildItem -Path $Bin -File -Include *.exe,*.dll,*.pdb | ForEach-Object {
    Copy-Item $_.FullName (Join-Path $GameSystemDir $_.Name) -Force
}
Write-Host "Kopiert nach: $GameSystemDir"
Write-Host "Start: $(Join-Path $GameSystemDir 'Unreal.exe')"