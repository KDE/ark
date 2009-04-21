include_directories(
                     ${CMAKE_CURRENT_SOURCE_DIR}/../..
                     ${CMAKE_CURRENT_BINARY_DIR}/../..
                   )

set(kerfuffle_singlefile_SRCS singlefileplugin.cpp)

#
# GZip files
#
macro_optional_find_package(ZLIB)

macro_log_feature(ZLIB_FOUND "zlib" "zlib is used for the .gz format" "http://www.zlib.net" FALSE "" "Required for the .gz format support in Ark")

if (ZLIB_FOUND)
	set(kerfuffle_libgz_SRCS gzplugin.cpp ${kerfuffle_singlefile_SRCS})

	kde4_add_plugin(kerfuffle_libgz ${kerfuffle_libgz_SRCS})

	target_link_libraries(kerfuffle_libgz ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} kerfuffle )

	install(TARGETS kerfuffle_libgz DESTINATION ${PLUGIN_INSTALL_DIR} )

	install( FILES kerfuffle_libgz.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
endif (ZLIB_FOUND)

#
# Bzip2 files
#
macro_optional_find_package(BZip2)

macro_log_feature(BZIP2_FOUND "bzip2" "bzip2 is used for the .bz2 format" "http://www.bzip2.org" FALSE "" "Required for the .bz2 format support in Ark")

if (BZIP2_FOUND)
	set(kerfuffle_libbz2_SRCS bz2plugin.cpp ${kerfuffle_singlefile_SRCS})

	kde4_add_plugin(kerfuffle_libbz2 ${kerfuffle_libbz2_SRCS})

	target_link_libraries(kerfuffle_libbz2 ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} kerfuffle )

	install(TARGETS kerfuffle_libbz2 DESTINATION ${PLUGIN_INSTALL_DIR} )

	install( FILES kerfuffle_libbz2.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
endif (BZIP2_FOUND)

#
# LZMA files
#
macro_optional_find_package(LibLZMA)

macro_log_feature(LIBLZMA_FOUND "LibLZMA" "Liblzma is used for the .xz and .lzma formats" "http://tukaani.org/xz/" FALSE "" "Required for the .xz and .lzma format support in Ark")

if (LIBLZMA_FOUND)
	set(kerfuffle_libxz_SRCS xzplugin.cpp ${kerfuffle_singlefile_SRCS})

	kde4_add_plugin(kerfuffle_libxz ${kerfuffle_libxz_SRCS})

	target_link_libraries(kerfuffle_libxz ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} kerfuffle )

	install(TARGETS kerfuffle_libxz DESTINATION ${PLUGIN_INSTALL_DIR} )

	install( FILES kerfuffle_libxz.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
endif (LIBLZMA_FOUND)
