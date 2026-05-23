# Install script for directory: E:/Development/Android/UE1/com.ast.unreal/app/src/main/cpp/UE1/Source

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/UE1Android")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "0")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "C:/Users/andiw/AppData/Local/Android/Sdk/ndk/27.0.12077973/toolchains/llvm/prebuilt/windows-x86_64/bin/llvm-objdump.exe")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/UE1/Core/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/UE1/Engine/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/UE1/Render/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/UE1/IpDrv/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/UE1/Fire/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/UE1/NSDLDrv/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/UE1/NOpenGLESDrv/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/UE1/SoundDrv/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/UE1/NOpenALDrv/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/UE1/Unreal/cmake_install.cmake")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/Debug/libCore.a;E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/Debug/libEngine.a;E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/Debug/libRender.a;E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/Debug/libIpDrv.a;E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/Debug/libFire.a;E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/Debug/libUnreal.so;E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/Debug/libNSDLDrv.a;E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/Debug/libNOpenGLESDrv.a;E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/Debug/libSoundDrv.a;E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/Debug/libNOpenALDrv.a")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/Debug" TYPE FILE FILES
    "E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/UE1/Core/libCore.a"
    "E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/UE1/Engine/libEngine.a"
    "E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/UE1/Render/libRender.a"
    "E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/UE1/IpDrv/libIpDrv.a"
    "E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/UE1/Fire/libFire.a"
    "E:/Development/Android/UE1/com.ast.unreal/app/build/intermediates/cxx/Debug/5m4t282m/obj/arm64-v8a/libUnreal.so"
    "E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/UE1/NSDLDrv/libNSDLDrv.a"
    "E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/UE1/NOpenGLESDrv/libNOpenGLESDrv.a"
    "E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/UE1/SoundDrv/libSoundDrv.a"
    "E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/UE1/NOpenALDrv/libNOpenALDrv.a"
    )
endif()

