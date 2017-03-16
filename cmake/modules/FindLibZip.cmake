# Find libzip library and headers
#
# The module defines the following variables:
#
# ::
#
#   LibZip_FOUND             - true if libzip was found
#   LibZip_INCLUDE_DIRS      - include search path
#   LibZip_INCLUDE_CONF_DIRS - include search path for zipconf.h
#   LibZip_LIBRARIES         - libraries to link
#   LibZip_VERSION           - libzip 3-component version number

find_path(LibZip_INCLUDE_DIRS
  NAMES zip.h
  PATHS ${GNUDIR}/include
)

# Contains the version of libzip:
find_path(LibZip_INCLUDE_CONF_DIRS
  NAMES zipconf.h
  PATHS /usr/lib/libzip/include /usr/local/lib/libzip/include
)

find_library(LibZip_LIBRARIES
  NAMES zip libzip
  PATHS ${GNUDIR}/lib
)

# Extract the version number from the header.
if(LibZip_INCLUDE_CONF AND EXISTS "${LibZip_INCLUDE_CONF}/zipconf.h")
  file(STRINGS "${LibZip_INCLUDE_CONF}/zipconf.h" libzip_version_str
       REGEX "^#[\t ]*define[\t ]+LIBZIP_VERSION_(MAJOR|MINOR|MICRO)[\t ]+[0-9]+$")
  unset(LIBZIP_VERSION_STRING)
  foreach(VPART MAJOR MINOR MICRO)
    foreach(VLINE ${libzip_version_str})
      if(VLINE MATCHES "^#[\t ]*define[\t ]+LIBZIP_VERSION_${VPART}[\t ]+([0-9]+)$")
        set(LIBZIP_VERSION_PART "${CMAKE_MATCH_1}")
        if(LIBZIP_VERSION_STRING)
          string(APPEND LIBZIP_VERSION_STRING ".${LIBZIP_VERSION_PART}")
        else()
          set(LIBZIP_VERSION_STRING "${LIBZIP_VERSION_PART}")
        endif()
        unset(LIBZIP_VERSION_PART)
      endif()
    endforeach()
  endforeach()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibZip
                                  FOUND_VAR LibZip_FOUND
                                  REQUIRED_VARS LibZip_LIBRARIES LibZip_INCLUDE_DIRS
                                  VERSION_VAR LIBZIP_VERSION_STRING
)
