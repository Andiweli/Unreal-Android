project(NOpenGLDrv C CXX)

if(NOT USE_SDL)
  message(FATAL_ERROR "NOpenGLDrv requires NSDLDrv to function.")
endif()

set(SRC_FILES "NOpenGLDrv.cpp" "${CMAKE_SOURCE_DIR}/../Thirdparty/glad/glad.c")

add_library(${PROJECT_NAME} ${LIB_TYPE} ${SRC_FILES})

target_include_directories(${PROJECT_NAME}
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/Inc
  PRIVATE
  ${CMAKE_SOURCE_DIR}/../Thirdparty/glad
  ${SDL2_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARY})

target_link_libraries(${PROJECT_NAME} Render Engine Core)

target_compile_definitions(${PROJECT_NAME} PRIVATE NOPENGLDRV_EXPORTS UPACKAGE_NAME=${PROJECT_NAME})
