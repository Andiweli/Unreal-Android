cmake_minimum_required(VERSION 3.22.1)
project(UE1Android LANGUAGES C CXX)

# UNREAL_ANDROID_API16_NATIVE_COMPAT_V212
option(UNREAL_ANDROID_API16_COMPAT "Build the normal Android/OUYA flavor down to API 16" OFF)

option(UNREAL_ANDROID_AUTOMOTIVE "Build the Android Automotive flavor" OFF)
if(UNREAL_ANDROID_AUTOMOTIVE)
    add_compile_definitions(UNREAL_ANDROID_AUTOMOTIVE=1)
else()
    add_compile_definitions(UNREAL_ANDROID_AUTOMOTIVE=0)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# UNREAL_ANDROID_16K_LINKER_V212 / UNREAL_ANDROID_STATIC_LIBCXX_V212
# NDK r23 is required for the API16 normal/OUYA flavor. libc++ is linked
# statically by Gradle so the old r23 libc++_shared.so cannot become the lone
# unaligned DSO; explicitly align every generated shared object for 16KB pages.
if(ANDROID)
    add_link_options(
        "-Wl,-z,max-page-size=16384"
        "-Wl,-z,common-page-size=16384"
    )
endif()

set(UE1_ROOT "${CMAKE_CURRENT_LIST_DIR}/UE1")
set(SDL2_ROOT "${CMAKE_CURRENT_LIST_DIR}/thirdparty/SDL2")
set(OPENAL_ROOT "${CMAKE_CURRENT_LIST_DIR}/thirdparty/openal-soft")

if(NOT EXISTS "${UE1_ROOT}/Source/CMakeLists.txt" OR NOT EXISTS "${SDL2_ROOT}/CMakeLists.txt" OR NOT EXISTS "${OPENAL_ROOT}/CMakeLists.txt")
    message(WARNING "Native sources are not prepared yet. Building small sync stubs; Gradle prepareSources will fetch UE1/SDL2/OpenAL before a real build.")
    add_library(SDL2 SHARED android/sync_stub.c)
    add_library(openal SHARED android/sync_stub.c)
    add_library(Unreal SHARED android/sync_stub.c)
    return()
endif()

# UE1_ANDROID_MUSIC_LIBXMP_PATCH_BEGIN
# Replaces the temporary xmp stub with real libxmp for UMX/module playback.
set(UE1_LIBXMP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libxmp")
if(NOT EXISTS "${UE1_LIBXMP_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "libxmp source missing at ${UE1_LIBXMP_DIR}. Run apply_music_patch.ps1 again.")
endif()

set(BUILD_STATIC ON CACHE BOOL "Build libxmp static" FORCE)
set(BUILD_SHARED OFF CACHE BOOL "Do not build libxmp shared" FORCE)
set(BUILD_LITE OFF CACHE BOOL "Use full libxmp, not lite" FORCE)
set(WITH_UNIT_TESTS OFF CACHE BOOL "Disable libxmp tests" FORCE)
set(LIBXMP_PIC ON CACHE BOOL "Build libxmp with PIC" FORCE)

add_subdirectory("${UE1_LIBXMP_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/libxmp" EXCLUDE_FROM_ALL)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/ue1_xmp_real_dummy.c" "void ue1_xmp_real_dummy(void) {}\n")
add_library(ue1_xmp_stub STATIC "${CMAKE_CURRENT_BINARY_DIR}/ue1_xmp_real_dummy.c")
target_link_libraries(ue1_xmp_stub PUBLIC xmp_static)
target_include_directories(ue1_xmp_stub PUBLIC "${UE1_LIBXMP_DIR}/include")
target_compile_definitions(ue1_xmp_stub PUBLIC WITH_LIBXMP=1 WITH_XMP=1)
# UE1_ANDROID_MUSIC_LIBXMP_PATCH_END
target_include_directories(ue1_xmp_stub PUBLIC android/include)

# SDL2 as shared lib, because SDLActivity loads libSDL2.so before libUnreal.so.
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
if(UNREAL_ANDROID_API16_COMPAT AND ANDROID_ABI STREQUAL "armeabi-v7a")
    # The API16/OUYA build cannot use Android's API18+ HID plumbing. Unreal's
    # native Android controller backend and SDL joystick path remain enabled.
    set(SDL_HIDAPI OFF CACHE BOOL "" FORCE)
    set(SDL_HIDAPI_LIBUSB OFF CACHE BOOL "" FORCE)
    set(SDL_HIDAPI_JOYSTICK OFF CACHE BOOL "" FORCE)

    # UNREAL_ANDROID_API16_HAPTIC_LINK_FIX_V213
    # Keep SDL's Android haptic backend enabled. SDL_android.c always exports
    # JNI haptic add/remove entry points that call Android_AddHaptic() and
    # Android_RemoveHaptic(); those symbols are implemented by
    # src/haptic/android/SDL_syshaptic.c only when SDL_HAPTIC is enabled.
    # The Android backend itself supports device hotplug from API 16, so this
    # is valid for OUYA/Jelly Bean and fixes the armeabi-v7a release link.
    set(SDL_HAPTIC ON CACHE BOOL "" FORCE)
endif()
add_subdirectory("${SDL2_ROOT}" "${CMAKE_BINARY_DIR}/SDL2")

# OpenAL Soft, trimmed for Android.
set(LIBTYPE SHARED CACHE STRING "" FORCE)
set(ALSOFT_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ALSOFT_UTILS OFF CACHE BOOL "" FORCE)
set(ALSOFT_TESTS OFF CACHE BOOL "" FORCE)
# OpenAL Soft 1.24.3 is deliberately pinned: it still uses C++17 (NDK r23
# compatible) and contains the Android 16KB-page build fix.
# ALSOFT_CONFIG is deprecated; use the replacement option.
set(ALSOFT_INSTALL_CONFIG OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL OFF CACHE BOOL "" FORCE)
set(ALSOFT_BACKEND_OPENSL ON CACHE BOOL "" FORCE)
set(ALSOFT_BACKEND_OBOE OFF CACHE BOOL "" FORCE)
if(UNREAL_ANDROID_API16_COMPAT)
    # Keep the unified Normal/OUYA native library linkable at API 16. OpenSL ES
    # is the supported backend for this floor; Automotive keeps the newer setup.
    set(ALSOFT_BACKEND_AAUDIO OFF CACHE BOOL "" FORCE)
    set(ALSOFT_REQUIRE_OPENSL ON CACHE BOOL "" FORCE)
endif()
add_subdirectory("${OPENAL_ROOT}" "${CMAKE_BINARY_DIR}/openal-soft")

# Keep UE1 warnings visible, but silence warning-only noise from bundled
# third-party dependencies when building with modern Android Clang.
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    if(TARGET xmp_static)
        target_compile_options(xmp_static PRIVATE -Wno-unused-function)
    endif()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    foreach(_ue1_thirdparty_cxx_target IN ITEMS OpenAL alsoft.common)
        if(TARGET ${_ue1_thirdparty_cxx_target})
            target_compile_options(${_ue1_thirdparty_cxx_target} PRIVATE -Wno-sign-conversion)
        endif()
    endforeach()
endif()

# Variables consumed by the upstream UE1 CMake files.
set(USE_SDL ON CACHE BOOL "" FORCE)
set(BUILD_EDITOR OFF CACHE BOOL "" FORCE)
set(BUILD_SOFTDRV OFF CACHE BOOL "" FORCE)
set(BUILD_NOPENGLDRV OFF CACHE BOOL "" FORCE)
set(BUILD_NOPENGLESDRV ON CACHE BOOL "" FORCE)
set(BUILD_NULLSOUNDDRV ON CACHE BOOL "" FORCE)
set(BUILD_NOPENALDRV ON CACHE BOOL "" FORCE)
set(BUILD_WINDRV OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC ON CACHE BOOL "" FORCE)

set(SDL2_INCLUDE_DIR "${SDL2_ROOT}/include" CACHE PATH "" FORCE)
if(TARGET SDL2::SDL2)
    set(SDL2_LIBRARY SDL2::SDL2 CACHE STRING "" FORCE)
else()
    set(SDL2_LIBRARY SDL2 CACHE STRING "" FORCE)
endif()

set(OPENAL_INCLUDE_DIR "${OPENAL_ROOT}/include" CACHE PATH "" FORCE)
if(TARGET OpenAL)
    set(OPENAL_LIBRARY OpenAL CACHE STRING "" FORCE)
else()
    set(OPENAL_LIBRARY openal CACHE STRING "" FORCE)
endif()
set(LIBXMP_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/android/include" CACHE PATH "" FORCE)
set(LIBXMP_LIBRARY ue1_xmp_stub CACHE STRING "" FORCE)

if(UNREAL_ANDROID_API16_COMPAT AND ANDROID_ABI STREQUAL "armeabi-v7a")
    add_compile_definitions(ANDROID_LEGACY_API16)
endif()

add_compile_definitions(
    PLATFORM_ANDROID
    UNREAL_ANDROID
    WITH_SDL2
    WITH_GLES2
    WITH_OPENAL
)

# UE1 normally assumes UE1/Source is the top-level CMake source directory.
# In Android Studio, this wrapper is the top-level source dir, so provide UE1's
# original cmake module path explicitly.
list(APPEND CMAKE_MODULE_PATH "${UE1_ROOT}/Source/cmake")

add_subdirectory("${UE1_ROOT}/Source" "${CMAKE_BINARY_DIR}/UE1")

# Android system libraries needed by the final shared object and some static UE1 modules.
if(TARGET Unreal)
    target_link_libraries(Unreal android log GLESv2 EGL)
endif()
if(TARGET NOpenGLESDrv)
    target_link_libraries(NOpenGLESDrv GLESv2 EGL android log)
endif()
if(TARGET NSDLDrv)
    target_link_libraries(NSDLDrv android log)
endif()

# UE1_ANDROID_MUSIC_LIBXMP_LINK_BEGIN
# Safety link for common final targets. Duplicates are harmless in CMake.
foreach(_ue1_music_target IN ITEMS Unreal main UE1)
    if(TARGET ${_ue1_music_target})
        target_link_libraries(${_ue1_music_target} ue1_xmp_stub)
    endif()
endforeach()
# UE1_ANDROID_MUSIC_LIBXMP_LINK_END
