project(Engine CXX)

file(GLOB SRC_FILES "Src/*.cpp")


# UNREAL_ANDROID_API16_ENGINE_EVENT_SAFE_FLAGS_V212
# Preserve the proven OUYA/API16 mover/event-chain stability while leaving renderer/SDL optimized.
if(ANDROID AND UNREAL_ANDROID_API16_COMPAT AND ANDROID_ABI STREQUAL "armeabi-v7a" AND NOT (CMAKE_BUILD_TYPE STREQUAL "Debug"))
  set(_UE1_API16_ENGINE_SAFE_FLAGS "-O0 -fno-strict-aliasing -fno-strict-overflow -fwrapv -fno-delete-null-pointer-checks -fno-omit-frame-pointer")
  set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/Src/UnScript.cpp" PROPERTIES COMPILE_FLAGS "${_UE1_API16_ENGINE_SAFE_FLAGS}")
  set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/Src/UnActor.cpp" PROPERTIES COMPILE_FLAGS "${_UE1_API16_ENGINE_SAFE_FLAGS}")
  set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/Src/UnLevAct.cpp" PROPERTIES COMPILE_FLAGS "${_UE1_API16_ENGINE_SAFE_FLAGS}")
  set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/Src/UnLevTic.cpp" PROPERTIES COMPILE_FLAGS "${_UE1_API16_ENGINE_SAFE_FLAGS}")
  set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/Src/UnMover.cpp" PROPERTIES COMPILE_FLAGS "${_UE1_API16_ENGINE_SAFE_FLAGS}")
endif()

add_library(${PROJECT_NAME} ${LIB_TYPE} ${SRC_FILES})

target_include_directories(${PROJECT_NAME}
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/Inc
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Src
)

target_link_libraries(${PROJECT_NAME} Core)

target_compile_definitions(${PROJECT_NAME} PRIVATE ENGINE_EXPORTS UPACKAGE_NAME=${PROJECT_NAME})
