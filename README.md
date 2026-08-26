<img width="2172" height="724" alt="image" src="https://github.com/Andiweli/Unreal-Android/blob/main/images/unreal-header.jpg" />

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

![OS](https://img.shields.io/badge/up%20to-Android%2016-green)
![Architecture](https://img.shields.io/badge/architecture-32/64bit-orange.svg)
![AI](https://img.shields.io/badge/AI-assisted%20coding-6e7781)
![Controller](https://img.shields.io/badge/Controls-Joypad/Touch/Keyb-blueviolet)
![Multiplayer](https://img.shields.io/badge/Multiplayer-local%20WiFi-blueviolet)
[![Support via PayPal](https://img.shields.io/badge/Support%20via-PayPal-0070BA?logo=paypal\&logoColor=white)](https://paypal.me/andiweli)



> [!NOTE]
> Unofficial fan port.  
> No game data included. Requires legally obtained Unreal v1.200 game files.  
> This project is not official and is not endorsed by Epic Games.


> [!IMPORTANT]
> Video games on smartphones are great, but not user-friendly if they require more than two thumbs to control. For this reason, this port is designed for controller input and does only offer basic touchscreen controls.
>
> This project is for preservation, experimentation and personal use only.  
> Unreal, Unreal Engine and related trademarks are owned by Epic Games.  
> This project is not affiliated with or endorsed by Epic Games.

---

## ▣ Screenshots

<img width="1920" height="1080" alt="unreal0" src="https://github.com/user-attachments/assets/d169d5e7-128f-465c-8a0a-fc133e896c11" />
<img width="1920" height="1080" alt="unreal1" src="https://github.com/user-attachments/assets/bff35f68-d400-4bea-af73-b65446853bda" />
<img width="1920" height="1080" alt="unreal2" src="https://github.com/user-attachments/assets/0658ea17-b7fc-40e9-bbee-1c038b24f852" />
<img width="1920" height="1080" alt="unreal3" src="https://github.com/user-attachments/assets/258ba31f-c790-46fa-bb57-1bff54a8db0c" />

---
 
## ◈ Features

- Android support for newer Android devices up to Android 16.
- OUYA (Android 4.x) legacy support - with a lower internal render resolution for better performance on legacy hardware.
- Improved Game Data Import – Unreal data can be imported via folder or ZIP selection and automatically installs to the app's data folder.
- Android 8+ Storage Access Fixed – SAF support added for modern Android versions where direct SD/file access is restricted.
- Legacy storage behavior friendly for old sideload devices (place game data on your microSD/Unreal folder).
- Local WiFi multiplayer and botmatches are available.
- Added touch controls for use without a controller.
- Added keyboard support (tested with Chromebook)

> [!NOTE]
> If you are experiencing problems with your player moving or turning slowly, please go to OPTIONS and Customize Controls.

## ▣ Requirements

- Android 8.0 or newer for the regular Android build.
- Chromebook with at least 4GB RAM and a powerful CPU (eg. MediaTek Kompano 520)
- OpenGL ES 2.0 capable GPU.
- A compatible device.
- Android-compatible game controller recommended.
- Unreal [v200 retail game data](https://archive.org/search?tab=all&query=unreal+v1.200).

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

## ❤️ Support

If you enjoy this project and would like to support my work, you can make a small contribution via PayPal.

Your support helps me spend more time maintaining existing projects, fixing bugs, improving compatibility, and working on new features.

[![Support via PayPal](https://img.shields.io/badge/Support%20via-PayPal-0070BA?logo=paypal\&logoColor=white)](https://paypal.me/andiweli)

Thank you for your support!

---

## ◎ Legal

Unreal, Unreal Engine and related names, assets and trademarks are property of Epic Games.
Portions of the materials used are trademarks and/or copyrighted works of Epic Games, Inc.

This repository does **not** include commercial game data.  
You must own a legal copy of Unreal to use this port.
This material is not official and is not endorsed by Epic.

All rights reserved by Epic.

Do not use this project for commercial purposes.
