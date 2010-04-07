macro_optional_find_package(LibArchive)

macro_log_feature(LIBARCHIVE_FOUND "LibArchive" "A library for dealing with a wide variety of archive file formats" "http://code.google.com/p/libarchive/" FALSE "" "Required for among others tar, tar.gz, tar.bz2 formats in Ark.")

configure_file(config-ark.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-ark.h )

if (LIBARCHIVE_FOUND)
    if( NOT HAVE_LIBARCHIVE_LZMA_SUPPORT OR NOT HAVE_LIBARCHIVE_XZ_SUPPORT )
        message(STATUS "Your libarchive does not have support for lzma and/or xz archives. libarchive >= 2.7.0 is recommended.")
    endif( NOT HAVE_LIBARCHIVE_LZMA_SUPPORT OR NOT HAVE_LIBARCHIVE_XZ_SUPPORT )
    add_subdirectory( libarchive )
endif (LIBARCHIVE_FOUND)

add_subdirectory( clirarplugin )
add_subdirectory( cli7zplugin )
add_subdirectory( clizipplugin )
add_subdirectory( libsinglefileplugin )
