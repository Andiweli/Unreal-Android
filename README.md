<img width="150" height="150" alt="unreal-icon" src="https://github.com/user-attachments/assets/8aa639be-16c3-4c1b-869b-3b1a8400a958" />

# Unreal Android

<p align="left">
  <a href="README.md">README</a>
  &nbsp;|&nbsp;
  <a href="ROADMAP.md">ROADMAP</a>
  &nbsp;|&nbsp;
  <a href="https://github.com/Andiweli/Unreal-Android/releases">DOWNLOAD</a>
  &nbsp;|&nbsp;
  <a href="https://github.com/Andiweli/Unreal-Android/releases/tag/v1.6.1">OUYA VERSION</a>
</p>

![Android 13](https://img.shields.io/badge/OS-up%20to%20Android%2013-green)
![ABI](https://img.shields.io/badge/ABI-armeabi--v7a/32bit-orange)
![AI](https://img.shields.io/badge/AI-assisted%20coding-6e7781)
![Controller](https://img.shields.io/badge/Controls-Touch/Controller-blueviolet)
![Multiplayer](https://img.shields.io/badge/Multiplayer-local%20WiFi-blueviolet)


> [!NOTE]
> Unofficial fan port.
> No game data included.
> Requires legally obtained Unreal v1.200 game files.
> This project is not official and is not endorsed by Epic Games.


> [!IMPORTANT]
> **This app is 32 bit only!** It won't install on your phone? Then it might only accept 64 bit apps. 
> There are no plans on making this app compatible with 64 bit only CPUs. 
> 
> Video games on smartphones are great, but not user-friendly if they require more than two thumbs to control. For this reason, this port is designed for controller input and does only offer basic touchscreen controls.
>
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
 
## ◈ Features

- Android support for newer Android devices up to Android 13 (no 64 bit devices).
- OUYA (Android 4.x) legacy support - with a lower internal render resolution for better performance on legacy hardware.
- Improved Game Data Import – Unreal data can be imported via folder or ZIP selection and automatically installs to the app's data folder.
- Android 8+ Storage Access Fixed – SAF support added for modern Android versions where direct SD/file access is restricted.
- Legacy storage behavior friendly for old sideload devices (place game data on your microSD/Unreal folder).
- Local WiFi multiplayer and botmatches are available.
- Added touch controls for use without a controller.

> [!NOTE]
> If you are experiencing problems with your player moving or turning slowly, please go to OPTIONS and Customize Controls.

## ▣ Requirements

- Android 8.0 or newer for the regular Android build.
- Android 4.x / API16 compatible device for the OUYA legacy build.
- OpenGL ES 2.0 capable GPU.
- Android-compatible game controller recommended.
- Unreal [v200 retail game data](https://www.google.com/search?q=archive.org+unreal+v1.200).

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

## ◎ Legal

Unreal, Unreal Engine and related names, assets and trademarks are property of Epic Games.
Portions of the materials used are trademarks and/or copyrighted works of Epic Games, Inc.

This repository does **not** include commercial game data.  
You must own a legal copy of Unreal to use this port.
This material is not official and is not endorsed by Epic.

All rights reserved by Epic.

Do not use this project for commercial purposes.
