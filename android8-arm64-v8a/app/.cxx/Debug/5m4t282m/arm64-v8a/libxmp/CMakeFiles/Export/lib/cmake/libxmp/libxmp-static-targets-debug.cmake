#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "libxmp::xmp_static" for configuration "Debug"
set_property(TARGET libxmp::xmp_static APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(libxmp::xmp_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libxmp.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS libxmp::xmp_static )
list(APPEND _IMPORT_CHECK_FILES_FOR_libxmp::xmp_static "${_IMPORT_PREFIX}/lib/libxmp.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
