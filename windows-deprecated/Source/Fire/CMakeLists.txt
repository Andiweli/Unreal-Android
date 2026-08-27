project(Fire CXX)

set(SRC_FILES
  "Src/UnFractal.cpp"
)

add_library(${PROJECT_NAME} ${LIB_TYPE} ${SRC_FILES})

target_include_directories(${PROJECT_NAME}
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/Inc
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Src
)

target_link_libraries(${PROJECT_NAME} Engine Core)

target_compile_definitions(${PROJECT_NAME} PRIVATE FIRE_EXPORTS UPACKAGE_NAME=${PROJECT_NAME})
# UE1_WINDOWS_FIRE_C4731_SUPPRESS_V24_BEGIN
# UE1's legacy Fire/Ice/Wet texture code contains old Win32 inline assembler.
# In MSVC x86 builds the assembler intentionally touches EBP in UnFractal.cpp,
# which produces warning C4731. The code builds and links correctly; suppress
# only this known warning for the Fire module instead of rewriting the asm.
if(MSVC AND WIN32)
    target_compile_options(${PROJECT_NAME} PRIVATE /wd4731)
endif()
# UE1_WINDOWS_FIRE_C4731_SUPPRESS_V24_END
