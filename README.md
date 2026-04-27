<img width="150" height="150" alt="unreal-icon" src="https://github.com/user-attachments/assets/8aa639be-16c3-4c1b-869b-3b1a8400a958" /> 

# Unreal-Android [WiP]

![Status](https://img.shields.io/badge/status-work%20in%20progress-orange)
![Platform](https://img.shields.io/badge/platform-Android-green)
![Engine](https://img.shields.io/badge/engine-Unreal%20Engine%201-blue)

Work-in-progress Android port of Unreal Engine 1 / Unreal v200 (1998).

This repository currently provides only an experimental Android version. It is not a finished game package.

Original Unreal game data is not included.

## Status

- Android port is work in progress
- No editor support

## Requirements

- Android 8.0 or newer (Android 4.x for OUYA is planned)
- Device with OpenGL ES 2.0 support
- Android-compatible game controller recommended
- Original Unreal v200 retail game data (or Unreal v205 demo data)

Copy the required Unreal game folders from a valid Unreal installation or from the Unreal v205 demo.

Required folders:

```text
Unreal/
+System/
+Maps/
+Textures/
+Sounds/
+Music/
```

## Game data location

The game directory 'Unreal' must be placed in one of these folders:

```text
/storage/emulated/0/Android/data/com.ast.unreal/files/
/storage/emulated/0/
/sdcard/
/storage/sdcard0/
/mnt/sdcard/
/mnt/usbdrive/
/mnt/usbdrive0/
/mnt/usb_storage/
```

Expected layout-examples:

```text
/storage/emulated/0/Android/data/com.ast.unreal/files/Unreal
/storage/emulated/0/Unreal
/sdcard/Unreal
/mnt/usbdrive/Unreal
```

## Android changes

This Android version includes several porting adjustments, including:

- Android Studio / Gradle / NDK project setup
- CMake integration for the native UE1 sources
- Android-specific SDL2 window, input and lifecycle handling
- OpenGL ES rendering adjustments
- Gamepad / Android controller input preparation
- Package and data path cleanup for `com.ast.unreal`
- Native dependency and build-system cleanup for Android

## Notes

This Android port is based on [fgsfdsfgs/UE1](https://github.com/fgsfdsfgs/UE1), which provides the Unreal Engine 1 v200 source with modifications for modern systems.

The upstream project is not a dedicated Linux ARM port. It is a shared Unreal Engine 1 v200 code base with several supported build targets, including Windows x86, Linux x86, Linux ARM32 and PSVita ARM32.

For this Android version, the general UE1 v200 code base is used as the foundation. The Linux ARM32 and PSVita/GLES2 parts are useful technical references for ARM, SDL2, OpenGL ES and mobile-style platform work, but Android is handled as its own target through Android Studio, Gradle, CMake, NDK and SDL2.

Unreal, Unreal Engine and related trademarks are owned by Epic Games. This project is not affiliated with or endorsed by Epic Games.

Do not use this project for commercial purposes.
