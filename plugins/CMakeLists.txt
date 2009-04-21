macro_optional_find_package(LibArchive)
macro_optional_find_package(LibZip)

macro_log_feature(LIBARCHIVE_FOUND "LibArchive" "A library for dealing with a wide variety of archive file formats" "http://people.freebsd.org/~kientzle/libarchive/" FALSE "" "Required for among others tar, tar.gz, tar.bz2 formats in Ark. Make sure it is compiled with gzip support.")

macro_log_feature(LIBZIP_FOUND "LibZip" "A library for reading, creating, and modifying ZIP archives" "http://www.nih.at/libzip" FALSE "" "Required for building ZIP support into Ark.")

if (LIBARCHIVE_FOUND)
	add_subdirectory( libarchive )
endif (LIBARCHIVE_FOUND)

if (LIBZIP_FOUND)
	add_subdirectory( libzipplugin )
endif (LIBZIP_FOUND)

add_subdirectory( clirarplugin )
add_subdirectory( cli7zplugin )
add_subdirectory( libsinglefileplugin )
add_subdirectory( unace )

if(NOT WIN32)
  # stdbool.h
  #add_subdirectory( bk )
  #add_subdirectory( rarplugin )
endif(NOT WIN32)

#add_subdirectory( karchiveplugin )
