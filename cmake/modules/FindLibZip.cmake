# Find libzip library and headers
#
# The module defines the following variables:
#
# ::
#
#   LibZip_FOUND             - true if libzip was found
#   LibZip_INCLUDE_DIRS      - include search path
#   LibZip_LIBRARIES         - libraries to link
#   LibZip_VERSION           - libzip 3-component version number

find_package(PkgConfig)
pkg_check_modules(PC_LIBZIP QUIET libzip)

set(LibZip_VERSION ${PC_LIBZIP_VERSION})

find_path(LibZip_INCLUDE_DIR zip.h
  HINTS ${PC_LIBZIP_INCLUDEDIR})

# Contains the version of libzip:
find_path(LibZip_INCLUDE_CONF_DIR zipconf.h
  HINTS ${PC_LIBZIP_INCLUDE_DIRS})

find_library(LibZip_LIBRARIES
  NAMES zip libzip
  HINTS ${PC_LIBZIP_LIBDIR})

set(LibZip_INCLUDE_DIRS ${LibZip_INCLUDE_DIR} ${LibZip_INCLUDE_CONF_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibZip
                                  FOUND_VAR LibZip_FOUND
                                  REQUIRED_VARS LibZip_LIBRARIES LibZip_INCLUDE_DIR LibZip_INCLUDE_CONF_DIR
                                  VERSION_VAR LibZip_VERSION)

if(LibZip_FOUND AND NOT TARGET LibZip::LibZip)
    add_library(LibZip::LibZip UNKNOWN IMPORTED)
    set_target_properties(LibZip::LibZip PROPERTIES
        IMPORTED_LOCATION "${LibZip_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${LibZip_INCLUDE_DIRS}"
    )
endif()

mark_as_advanced(LibZip_INCLUDE_DIR LibZip_INCLUDE_CONF_DIR)
