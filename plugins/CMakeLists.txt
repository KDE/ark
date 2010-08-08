macro_optional_find_package(LibArchive)

if (LIBARCHIVE_FOUND)
    if( HAVE_LIBARCHIVE_READ_DISK_API )
        if( NOT HAVE_LIBARCHIVE_LZMA_SUPPORT OR NOT HAVE_LIBARCHIVE_XZ_SUPPORT )
            message(STATUS "Your libarchive does not have support for lzma and/or xz archives. libarchive >= 2.7.0 is recommended.")
        endif( NOT HAVE_LIBARCHIVE_LZMA_SUPPORT OR NOT HAVE_LIBARCHIVE_XZ_SUPPORT )
        add_subdirectory( libarchive )
    else( HAVE_LIBARCHIVE_READ_DISK_API )
        # Remove the cached variables from FindLibArchive.cmake
        unset( LIBARCHIVE_FOUND )
        unset( LIBARCHIVE_INCLUDE_DIR )
        unset( LIBARCHIVE_LIBRARY )
        message(STATUS "Your libarchive does not have support for archive_read_disk api. libarchive >= 2.6.0 is needed.")
    endif( HAVE_LIBARCHIVE_READ_DISK_API )
endif (LIBARCHIVE_FOUND)

macro_log_feature(LIBARCHIVE_FOUND "LibArchive" "A library for dealing with a wide variety of archive file formats" "http://code.google.com/p/libarchive/" FALSE "" "Required for among others tar, tar.gz, tar.bz2 formats in Ark.")

add_subdirectory( clirarplugin )
add_subdirectory( cli7zplugin )
add_subdirectory( clizipplugin )
add_subdirectory( libsinglefileplugin )
