project(WinDrv CXX)

if(NOT TARGET_IS_WINDOWS)
  message(FATAL_ERROR "WinDrv is for Windows only.")
endif()

set(SRC_FILES
  "Res/WinDrvRes.rc"
  "WinViewport.cpp"
  "WinDrv.cpp"
)

add_library(${PROJECT_NAME} ${LIB_TYPE} ${SRC_FILES})

target_include_directories(${PROJECT_NAME}
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/Inc
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Src
)

target_link_libraries(${PROJECT_NAME} Window Engine Core)

target_link_libraries(${PROJECT_NAME} winmm dxguid)

target_compile_definitions(${PROJECT_NAME} PRIVATE WINDRV_EXPORTS UPACKAGE_NAME=${PROJECT_NAME})
