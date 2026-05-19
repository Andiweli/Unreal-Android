<img width="150" height="150" alt="unreal-icon" src="https://github.com/user-attachments/assets/8aa639be-16c3-4c1b-869b-3b1a8400a958" />

# Unreal Android

![Platform](https://img.shields.io/badge/platform-Android-green)
![Engine](https://img.shields.io/badge/engine-Unreal%20Engine%201-blue)
![Renderer](https://img.shields.io/badge/renderer-OpenGL%20ES%202.0-lightgrey)
![ABI](https://img.shields.io/badge/ABI-armeabi--v7a-orange)
![Controller](https://img.shields.io/badge/controller-supported-blueviolet)
![Multiplayer](https://img.shields.io/badge/multiplayer-local%20WiFi-blueviolet)

**Unreal-Android** is an Android port of **Unreal / Unreal v200 (1998)** based on the classic **Unreal Engine 1** source code.  
The goal of this project is to make Epic's original Unreal playable on modern Android devices and on legacy Android hardware such as the **OUYA console**, while keeping the look and feel of the original PC release.

Original Unreal game data is **not included**. You need a valid Unreal installation v200 or the Unreal v205 demo data.

> [!IMPORTANT]
> This project is for preservation, experimentation and personal use only.  
> Unreal, Unreal Engine and related trademarks are owned by Epic Games.  
> This project is not affiliated with or endorsed by Epic Games.

---

<p align="center">
<a href="https://github.com/user-attachments/assets/58f261b8-8403-4400-a6be-3944bd51f770"><img src="https://i.ibb.co/5tLykYx/screen-Video.jpg" alt="screen-Video" border="0" width="320" height="180"></a>
<a href="https://ibb.co/tpv15z3K"><img src="https://i.ibb.co/0y6z3KQ2/screen-A.png" alt="screen-A" border="0" width="320" height="180"></a>
<a href="https://ibb.co/zVsG0bJP"><img src="https://i.ibb.co/RT2zVPyQ/screen-B.png" alt="screen-B" border="0" width="320" height="180"></a>
</p>

---
 
## ◈ Features and improvements

This Android version contains multiple porting and gameplay improvements compared to the first experimental Android builds:

- Android 8.0+ support for newer Android devices.
- OUYA (Android 4.x) legacy support - with a lower internal render resolution for better performance on legacy hardware.
- Improved Game Data Import – Unreal data can be imported via folder or ZIP selection and automatically installs to the app's data folder.
- Android 11+ Storage Access Fixed – SAF support added for modern Android versions where direct SD/file access is restricted.
- Improved Brightness and Gamma – Separate brightness/gamma logic with improved brightening of dark level areas.
- Local WiFi multiplayer and botmatches are available.

> [!NOTE]
> If you are experiencing problems with your player moving or turning slowly, please go to OPTIONS and Customize Controls.

## ▣ Requirements

- Android 8.0 or newer for the regular Android build.
- Android 4.x / API16 compatible device for the OUYA legacy build.
- OpenGL ES 2.0 capable GPU.
- Android-compatible game controller recommended.
- Original Unreal [v200 retail game data](https://archive.org/download/epicmegagames-unreal-v200/EpicMegagames-Unreal-v200.zip).

> [!IMPORTANT]
> ## Installation procedere
> 1. Copy the required Unreal game .zip file to your microSD card or extract it to a folder like "Downloads".  
> 2. At first start, the game asks for this .zip file or a folder where it is extracted.  
> 3. Installation takes a while, when finished the game starts.

Required folders:

```text
Unreal/
├── System/
├── Maps/
├── Textures/
├── Sounds/
├── Shared/
└── Music/
```

---

## ▣ Game data notes

Game data is not bundled with this repository.

You need to provide your own legal copy of Unreal 1998.  
The Android installer checks for the required folders:

```text
System
Maps
Textures
Sounds
Music
```

If these folders are missing, the game will not start and the installer screen will ask you to select a valid folder or ZIP file.

---

## ◎ Notes

This Android port is based on [fgsfdsfgs/UE1](https://github.com/fgsfdsfgs/UE1), which provides the Unreal Engine 1 v200 source with modifications for modern systems.

The upstream project is not a dedicated Linux ARM port. It is a shared Unreal Engine 1 v200 code base with several supported build targets, including Windows x86, Linux x86, Linux ARM32 and PSVita ARM32.

For this Android version, the general UE1 v200 code base is used as the foundation. The Linux ARM32 and PSVita/GLES2 parts are useful technical references for ARM, SDL2, OpenGL ES and mobile-style platform work, but Android is handled as its own target through Android Studio, Gradle, CMake, NDK and SDL2.

Unreal, Unreal Engine and related trademarks are owned by Epic Games. This project is not affiliated with or endorsed by Epic Games.

Do not use this project for commercial purposes.
