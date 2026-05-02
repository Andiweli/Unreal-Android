project(Core CXX)

set(SRC_FILES
  "Src/UnCache.cpp"
  "Src/UnClass.cpp"
  "Src/UnCorSc.cpp"
  "Src/UnFile.cpp"
  "Src/UnMem.cpp"
  "Src/UnName.cpp"
  "Src/UnObj.cpp"
  "Src/UnOutDev.cpp"
  "Src/UnPlat.cpp"
  "Src/UnProp.cpp"
  "Src/UnConfig.cpp"
  "Src/UnThread.cpp"
  "Src/Core.cpp"
)

if(NOT TARGET_IS_WINDOWS)
  list(APPEND SRC_FILES "Src/UnGUID.cpp")
endif()

if(BUILD_STATIC)
  list(APPEND SRC_FILES "Src/UnStaticExports.cpp")
endif()

if(NOT MSVC AND NOT (CMAKE_BUILD_TYPE STREQUAL "Debug"))
  # FIXME: the very funny reference juggling UE does with FFrame::Step() confuses gcc,
  # so we'll lower optimization level for the core UnrealScript interpreter for now
  set_source_files_properties("Src/UnCorSc.cpp" PROPERTIES COMPILE_FLAGS -Og)
  set_source_files_properties("Src/UnProp.cpp" PROPERTIES COMPILE_FLAGS -Og)
endif()


# OUYA_API16_SCRIPTVM_SAFE_FLAGS_BEGIN
# Override after any earlier per-file flags:
# On OUYA/API16/Tegra3 the old UE1 UnrealScript VM/property interpreter is
# sensitive in Release. Button-called lifts exercise foreach/event/state chains.
if(ANDROID AND ANDROID_LEGACY_API16 AND NOT (CMAKE_BUILD_TYPE STREQUAL "Debug"))
  set(_UE1_OUYA_CORE_SCRIPTVM_SAFE_FLAGS "-O0 -fno-strict-aliasing -fno-strict-overflow -fwrapv -fno-delete-null-pointer-checks -fno-omit-frame-pointer")
  set_source_files_properties("Src/UnCorSc.cpp" PROPERTIES COMPILE_FLAGS "${_UE1_OUYA_CORE_SCRIPTVM_SAFE_FLAGS}")
  set_source_files_properties("Src/UnProp.cpp" PROPERTIES COMPILE_FLAGS "${_UE1_OUYA_CORE_SCRIPTVM_SAFE_FLAGS}")
endif()
# OUYA_API16_SCRIPTVM_SAFE_FLAGS_END

add_library(${PROJECT_NAME} ${LIB_TYPE} ${SRC_FILES})

target_include_directories(${PROJECT_NAME}
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/Inc
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Src
)

target_compile_definitions(${PROJECT_NAME} PRIVATE CORE_EXPORTS UPACKAGE_NAME=${PROJECT_NAME})

if(USE_SDL)
  target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARY})
  target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIR})
endif()
