# UE1 1.2.0 Windows / Visual Studio Port v5

Dieses Paket aktualisiert den bisherigen Windows-/VS-Port, behÃƒÆ’Ã‚Â¤lt die v4-MSVC-Core-Fixes bei und behebt zusÃƒÆ’Ã‚Â¤tzlich den Resource-Compiler-Fehler RC1015 wegen fehlendem afxres.h.

## Enthalten

- Visual-Studio-2026- und Visual-Studio-2022-CMake-Presets
- Win32/x86 Presets fÃƒÆ’Ã‚Â¼r SDL2 + NOpenGLDrv + OpenAL
- NullSound-Fallback
- Skript zum Generieren und ÃƒÆ’Ã¢â‚¬â€œffnen der echten `.sln`: `windows\generate_and_open_vs2026_solution.bat`
- MSVC-Core-Fix:
  - globale UE1-`operator new/delete`-Ersatzdefinitionen werden unter MSVC nicht mehr als DLL-Export/Import deklariert/definiert
  - `__builtin_trap()` wird durch `UE1_DEBUG_TRAP()` ersetzt; unter MSVC nutzt das `__debugbreak()`
- Resource-Compiler-Fix:
  - `Unreal/Src/Res/LaunchRes.rc`: `afxres.h` wird durch `winres.h` ersetzt

## Anwendung

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\apply_windows_vs_port_v5.ps1 -Root "E:\Development\VisualStudio\UE1\com.ast.unreal"
```

Danach sauber neu konfigurieren/bauen. Am besten den alten Buildordner lÃƒÆ’Ã‚Â¶schen:

```powershell
Remove-Item -Recurse -Force "E:\Development\VisualStudio\UE1\com.ast.unreal\app\src\main\cpp\UE1\Builds\\_cmake\\\vs2026-win32-sdl-opengl" -ErrorAction SilentlyContinue
& "E:\Development\VisualStudio\UE1\com.ast.unreal\app\src\main\cpp\UE1\windows\build_vs2026_win32_relwithdebinfo.bat"
```

## Visual Studio direkt ÃƒÆ’Ã‚Â¶ffnen

Nicht den Android-Projektroot ÃƒÆ’Ã‚Â¶ffnen. Entweder:

1. `windows\generate_and_open_vs2026_solution.bat` starten und danach die erzeugte Solution nutzen:
   `Builds\\_cmake\\\vs2026-win32-sdl-opengl\UE1.sln`

oder:

2. In Visual Studio: `Datei -> Ordner ÃƒÆ’Ã‚Â¶ffnen` und exakt diesen Ordner ÃƒÆ’Ã‚Â¶ffnen:
   `app\src\main\cpp\UE1\Source`

Win32/x86 ist absichtlich gesetzt. x64 bitte vorerst nicht verwenden.