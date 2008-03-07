macro_optional_find_package(LibZip)
macro_optional_find_package(LibArchive)

macro_log_feature(LIBZIP_FOUND "LibZip" "A library for reading, creating, and modifying ZIP archives" "http://www.nih.at/libzip" FALSE "" "Required for building ZIP support into Ark.")

macro_log_feature(LIBARCHIVE_FOUND "LibArchive" "A library for dealing with a wide variety of archive file formats" "http://people.freebsd.org/~kientzle/libarchive/" FALSE "" "Adds support for a variety of archive formats to Ark")

if (LIBARCHIVE_FOUND)
	add_subdirectory( libarchive )
endif (LIBARCHIVE_FOUND)

if (LIBZIP_FOUND)
	add_subdirectory( libzipplugin )
endif (LIBZIP_FOUND)

if(NOT WIN32)
  # stdbool.h
  add_subdirectory( bk )
  add_subdirectory( rarplugin )
endif(NOT WIN32)

#add_subdirectory( karchiveplugin )
