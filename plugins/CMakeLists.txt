#if (LIBARCHIVE_FOUND)
#	add_subdirectory( libarchive )
#endif (LIBARCHIVE_FOUND)

if (LIBZIP_FOUND)
	add_subdirectory( libzipplugin )
endif (LIBZIP_FOUND)

if(NOT WIN32)
  # stdbool.h
  add_subdirectory( bk )
endif(NOT WIN32)

add_subdirectory( karchiveplugin )
