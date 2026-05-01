<img width="150" height="150" alt="unreal-icon" src="https://github.com/user-attachments/assets/8aa639be-16c3-4c1b-869b-3b1a8400a958" />

# Unreal-Android

![Platform](https://img.shields.io/badge/platform-Android-green)
![Engine](https://img.shields.io/badge/engine-Unreal%20Engine%201-blue)
![Renderer](https://img.shields.io/badge/renderer-OpenGL%20ES%202.0-lightgrey)
![Multiplayer](https://img.shields.io/badge/multiplayer-local%20WiFi-blueviolet)

**Unreal-Android** is an Android port of **Unreal / Unreal v200 (1998)** based on the classic **Unreal Engine 1** source code.  
The goal of this project is to make Epic's original Unreal playable on modern Android devices and on legacy Android hardware such as the **OUYA console**, while keeping the look and feel of the original PC release.

Original Unreal game data is **not included**. You need a valid Unreal installation v200 or the Unreal v205 demo data.

> [!IMPORTANT]
> It really only works with v200 or v205, no other editions or releases like "Unreal Gold" or any. Search on archive.org. 

## ◆ Status

- Android 8.0+ support for newer Android devices.
- OUYA / Android 4.x legacy support with a low internal render resolution for better performance on legacy hardware.
- OUYA rendering is tuned for **960x540** to reduce GPU/CPU load while keeping fullscreen output.
- Local WiFi multiplayer and botmatches are available.
- Original Unreal v200 retail data and Unreal v205 demo data are supported.

<p align="center">
  <a href="https://github.com/user-attachments/assets/58f261b8-8403-4400-a6be-3944bd51f770">
    <img width="640" height="360" alt="Unreal Android gameplay video showing Unreal Engine 1 running on Android" src="https://github.com/user-attachments/assets/d74b4628-5d7a-45c5-89ef-7b15c8c6005a" />
  </a>
</p>

<p align="center">
  <img width="640" height="360" alt="Unreal Android screenshot with classic Unreal gameplay" src="https://github.com/user-attachments/assets/c262f8db-8353-4216-8826-9bf91bd10c08" />
</p>

<p align="center">
  <img width="640" height="360" alt="Unreal Engine 1 Android port screenshot" src="https://github.com/user-attachments/assets/869dba74-24ae-4222-8a12-cb5e02965d4c" />
</p>

## ◈ Features and improvements

This Android version contains multiple porting and gameplay improvements compared to the first experimental Android builds:

- Native Android project setup using Android Studio, Gradle, CMake and the Android NDK.
- SDL2 based Android window, input, lifecycle and controller handling.
- OpenGL ES 2.0 renderer adjustments for mobile GPUs.
- Improved texture handling and rendering stability.
- Fixes for visual effects such as explosions, water, waterfalls and water ripples.
- Improved data path detection for internal storage, SD card paths and USB storage.
- OUYA/API16 legacy compatibility for Android 4.1.2 devices.
- 960x540 OUYA render setup for smoother gameplay on legacy hardware.
- Local WiFi multiplayer support, including LAN discovery, direct IP joining and botgames.
- LAN multiplayer join fixes for Android hosts that advertise loopback addresses.

## ▣ Requirements

- Android 8.0 or newer for the regular Android build.
- Android 4.x / API16 compatible device for the OUYA legacy build.
- OpenGL ES 2.0 capable GPU.
- Android-compatible game controller recommended.
- Original Unreal v200 retail game data or Unreal v205 demo data.

Copy the required Unreal game folders from a valid Unreal installation or from the Unreal v205 demo.

Required folders:

```text
Unreal/
+System/
+Maps/
+Textures/
+Sounds/
+Shared/
+Music/
+Help/
```

### Game data location

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

Expected layout-examples (please place the above mentions sub-folders inside):

```text
/storage/emulated/0/Android/data/com.ast.unreal/files/Unreal
/storage/emulated/0/Unreal
/sdcard/Unreal
/mnt/usbdrive/Unreal
```

## ◎ Notes

This Android port is based on [fgsfdsfgs/UE1](https://github.com/fgsfdsfgs/UE1), which provides the Unreal Engine 1 v200 source with modifications for modern systems.

The upstream project is not a dedicated Linux ARM port. It is a shared Unreal Engine 1 v200 code base with several supported build targets, including Windows x86, Linux x86, Linux ARM32 and PSVita ARM32.

For this Android version, the general UE1 v200 code base is used as the foundation. The Linux ARM32 and PSVita/GLES2 parts are useful technical references for ARM, SDL2, OpenGL ES and mobile-style platform work, but Android is handled as its own target through Android Studio, Gradle, CMake, NDK and SDL2.

Unreal, Unreal Engine and related trademarks are owned by Epic Games. This project is not affiliated with or endorsed by Epic Games.

Do not use this project for commercial purposes.
