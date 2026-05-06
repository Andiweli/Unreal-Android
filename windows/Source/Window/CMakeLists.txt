project(Window CXX)

if(NOT TARGET_IS_WINDOWS)
  message(FATAL_ERROR "Window is for Windows only.")
endif()

set(SRC_FILES
  "Src/Res/WindowRes.rc"
  "Src/Main.cpp"
  "Src/Window.cpp"
)

add_library(${PROJECT_NAME} ${LIB_TYPE} ${SRC_FILES})

target_include_directories(${PROJECT_NAME}
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/Inc
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Src
)

target_link_libraries(${PROJECT_NAME} Engine Core)

target_link_libraries(${PROJECT_NAME} comctl32)

target_compile_definitions(${PROJECT_NAME} PRIVATE WINDOW_EXPORTS UPACKAGE_NAME=${PROJECT_NAME})
