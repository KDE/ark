macro_optional_find_package(LibArchive)

macro_log_feature(LIBARCHIVE_FOUND "LibArchive" "A library for dealing with a wide variety of archive file formats" "http://people.freebsd.org/~kientzle/libarchive/" FALSE "" "Required for among others tar, tar.gz, tar.bz2 formats in Ark. Make sure it is compiled with gzip support.")

if (LIBARCHIVE_FOUND)
	add_subdirectory( libarchive )
endif (LIBARCHIVE_FOUND)

add_subdirectory( clirarplugin )
add_subdirectory( cli7zplugin )
add_subdirectory( clizipplugin )
add_subdirectory( libsinglefileplugin )
