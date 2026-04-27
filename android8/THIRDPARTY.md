# Third Party Notices

This Android project downloads/builds third-party source packages during `prepareSources`.

## fgsfdsfgs/UE1

- Source: `https://github.com/fgsfdsfgs/UE1`
- Purpose: Unreal Engine 1 v200 source port baseline
- License/status: See upstream repository. The upstream README states this is based on UE1 v200 source with proprietary game assets removed and is not affiliated with or endorsed by Epic Games.
- Game data: not included.

## SDL2

- Source: `https://www.libsdl.org/release/SDL2-2.32.10.zip`
- Purpose: Android activity bridge, windowing, OpenGL ES context, controller input
- License: zlib license

## OpenAL Soft

- Source: `https://github.com/kcat/openal-soft/archive/refs/tags/1.25.1.zip`
- Purpose: OpenAL-compatible Android audio backend
- License: LGPL-2.0-or-later, see upstream

## libxmp stub

- Source: local minimal compatibility stub in `app/src/main/cpp/android/xmp_stub.c`
- Purpose: Allows `NOpenALDrv` to compile while music decoding is intentionally disabled until real libxmp is integrated.
- License: project-local generated glue code

## Android Gradle Plugin / Gradle

- AGP: `8.13.2`
- Gradle distribution metadata: `8.13`
- Purpose: Android Studio Otter 2 Feature Drop compatible build
