project(Engine CXX)

file(GLOB SRC_FILES "Src/*.cpp")


# OUYA_API16_ENGINE_MOVER_EVENT_SAFE_FLAGS_BEGIN
# OUYA/API16 Release-only lift/call-button safety:
# Keep renderer/SDL/GLES optimized, but compile the old UE1 gameplay event chain
# conservatively. Button-called lifts exercise Trigger.Touch, foreach AllActors,
# ProcessEvent, MoveActor/encroachment and mover state code.
if(ANDROID AND ANDROID_LEGACY_API16 AND NOT (CMAKE_BUILD_TYPE STREQUAL "Debug"))
  set(_UE1_OUYA_ENGINE_EVENT_SAFE_FLAGS "-O0 -fno-strict-aliasing -fno-strict-overflow -fwrapv -fno-delete-null-pointer-checks -fno-omit-frame-pointer")
  set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/Src/UnScript.cpp" PROPERTIES COMPILE_FLAGS "${_UE1_OUYA_ENGINE_EVENT_SAFE_FLAGS}")
  set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/Src/UnActor.cpp" PROPERTIES COMPILE_FLAGS "${_UE1_OUYA_ENGINE_EVENT_SAFE_FLAGS}")
  set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/Src/UnLevAct.cpp" PROPERTIES COMPILE_FLAGS "${_UE1_OUYA_ENGINE_EVENT_SAFE_FLAGS}")
  set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/Src/UnLevTic.cpp" PROPERTIES COMPILE_FLAGS "${_UE1_OUYA_ENGINE_EVENT_SAFE_FLAGS}")
  set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/Src/UnMover.cpp" PROPERTIES COMPILE_FLAGS "${_UE1_OUYA_ENGINE_EVENT_SAFE_FLAGS}")
endif()
# OUYA_API16_ENGINE_MOVER_EVENT_SAFE_FLAGS_END

add_library(${PROJECT_NAME} ${LIB_TYPE} ${SRC_FILES})

target_include_directories(${PROJECT_NAME}
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/Inc
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Src
)

target_link_libraries(${PROJECT_NAME} Core)

target_compile_definitions(${PROJECT_NAME} PRIVATE ENGINE_EXPORTS UPACKAGE_NAME=${PROJECT_NAME})
