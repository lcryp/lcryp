#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "libzmq-static" for configuration "Release"
set_property(TARGET libzmq-static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(libzmq-static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C;CXX;RC"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "ws2_32;advapi32;rpcrt4;iphlpapi"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libzmq-mt-s-4_3_4.lib"
  )

list(APPEND _cmake_import_check_targets libzmq-static )
list(APPEND _cmake_import_check_files_for_libzmq-static "${_IMPORT_PREFIX}/lib/libzmq-mt-s-4_3_4.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
