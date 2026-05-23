@echo off
"C:\\Users\\andiw\\AppData\\Local\\Android\\Sdk\\cmake\\3.22.1\\bin\\cmake.exe" ^
  "-HE:\\Development\\Android\\UE1\\com.ast.unreal\\app\\src\\main\\cpp" ^
  "-DCMAKE_SYSTEM_NAME=Android" ^
  "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON" ^
  "-DCMAKE_SYSTEM_VERSION=23" ^
  "-DANDROID_ABI=arm64-v8a" ^
  "-DCMAKE_ANDROID_ARCH_ABI=arm64-v8a" ^
  "-DANDROID_NDK=C:\\Users\\andiw\\AppData\\Local\\Android\\Sdk\\ndk\\27.0.12077973" ^
  "-DCMAKE_ANDROID_NDK=C:\\Users\\andiw\\AppData\\Local\\Android\\Sdk\\ndk\\27.0.12077973" ^
  "-DCMAKE_TOOLCHAIN_FILE=C:\\Users\\andiw\\AppData\\Local\\Android\\Sdk\\ndk\\27.0.12077973\\build\\cmake\\android.toolchain.cmake" ^
  "-DCMAKE_MAKE_PROGRAM=C:\\Users\\andiw\\AppData\\Local\\Android\\Sdk\\cmake\\3.22.1\\bin\\ninja.exe" ^
  "-DCMAKE_CXX_FLAGS=-std=c++17 -fexceptions -frtti -DUNREAL_ANDROID_DUAL_ABI=1 -DUNREAL_ANDROID_VERSION_NAME=1.3.2-Arm32/64" ^
  "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=E:\\Development\\Android\\UE1\\com.ast.unreal\\app\\build\\intermediates\\cxx\\Debug\\5m4t282m\\obj\\arm64-v8a" ^
  "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=E:\\Development\\Android\\UE1\\com.ast.unreal\\app\\build\\intermediates\\cxx\\Debug\\5m4t282m\\obj\\arm64-v8a" ^
  "-DCMAKE_BUILD_TYPE=Debug" ^
  "-BE:\\Development\\Android\\UE1\\com.ast.unreal\\app\\.cxx\\Debug\\5m4t282m\\arm64-v8a" ^
  -GNinja ^
  "-DANDROID_STL=c++_shared" ^
  "-DANDROID_PLATFORM=android-23"
