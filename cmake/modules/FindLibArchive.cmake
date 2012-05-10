# - Try to find libarchive
# Once done this will define
#
#  LIBARCHIVE_FOUND - system has libarchive
#  LIBARCHIVE_INCLUDE_DIR - the libarchive include directory
#  LIBARCHIVE_LIBRARY - Link this to use libarchive
#  HAVE_LIBARCHIVE_GZIP_SUPPORT - whether libarchive has been compiled with gzip support
#  HAVE_LIBARCHIVE_LZMA_SUPPORT - whether libarchive has been compiled with lzma support
#  HAVE_LIBARCHIVE_XZ_SUPPORT - whether libarchive has been compiled with xz support
#  HAVE_LIBARCHIVE_RPM_SUPPORT - whether libarchive has been compiled with rpm support
#  HAVE_LIBARCHIVE_CAB_SUPPORT - whether libarchive has been compiled with cab support
#
# Copyright (c) 2006, Pino Toscano, <toscano.pino@tiscali.it>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(CheckLibraryExists)

if (LIBARCHIVE_LIBRARY AND LIBARCHIVE_INCLUDE_DIR)
  # in cache already
  set(LIBARCHIVE_FOUND TRUE)
else (LIBARCHIVE_LIBRARY AND LIBARCHIVE_INCLUDE_DIR)
  find_path(LIBARCHIVE_INCLUDE_DIR archive.h
    ${GNUWIN32_DIR}/include
  )

  find_library(LIBARCHIVE_LIBRARY NAMES archive libarchive archive2 libarchive2
    PATHS
    ${GNUWIN32_DIR}/lib
  )

  if (LIBARCHIVE_LIBRARY)
    check_library_exists(${LIBARCHIVE_LIBRARY} archive_write_set_compression_gzip   "" HAVE_LIBARCHIVE_GZIP_SUPPORT)
    check_library_exists(${LIBARCHIVE_LIBRARY} archive_write_set_compression_lzma   "" HAVE_LIBARCHIVE_LZMA_SUPPORT)
    check_library_exists(${LIBARCHIVE_LIBRARY} archive_write_set_compression_xz     "" HAVE_LIBARCHIVE_XZ_SUPPORT)
    check_library_exists(${LIBARCHIVE_LIBRARY} archive_read_support_compression_rpm "" HAVE_LIBARCHIVE_RPM_SUPPORT)
    check_library_exists(${LIBARCHIVE_LIBRARY} archive_read_disk_entry_from_file    "" HAVE_LIBARCHIVE_READ_DISK_API)
    check_library_exists(${LIBARCHIVE_LIBRARY} archive_read_support_format_cab      "" HAVE_LIBARCHIVE_CAB_SUPPORT)
  endif (LIBARCHIVE_LIBRARY)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(LibArchive DEFAULT_MSG LIBARCHIVE_INCLUDE_DIR LIBARCHIVE_LIBRARY HAVE_LIBARCHIVE_GZIP_SUPPORT)

  mark_as_advanced(LIBARCHIVE_INCLUDE_DIR LIBARCHIVE_LIBRARY HAVE_LIBARCHIVE_GZIP_SUPPORT HAVE_LIBARCHIVE_LZMA_SUPPORT HAVE_LIBARCHIVE_RPM_SUPPORT HAVE_LIBARCHIVE_CAB_SUPPORT)
endif (LIBARCHIVE_LIBRARY AND LIBARCHIVE_INCLUDE_DIR)
