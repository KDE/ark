set(kerfuffle_singlefile_SRCS singlefileplugin.cpp)
set(SUPPORTED_LIBSINGLEFILE_MIMETYPES "")

# This MIME type was originally set in ark.desktop but is does not
# seem to be supported anywhere.  Assuming that, if it were supported,
# it would be here.
set(SUPPORTED_LIBSINGLEFILE_MIMETYPES "${SUPPORTED_LIBSINGLEFILE_MIMETYPES}application/x-compress;")

#
# GZip files
#
macro_optional_find_package(ZLIB)

macro_log_feature(ZLIB_FOUND "ZLib" "The Zlib compression library" "http://www.zlib.net" FALSE "" "Required for the .gz format support in Ark")

if (ZLIB_FOUND)
	set(kerfuffle_libgz_SRCS gzplugin.cpp ${kerfuffle_singlefile_SRCS})
	set(SUPPORTED_LIBSINGLEFILE_MIMETYPES "${SUPPORTED_LIBSINGLEFILE_MIMETYPES}application/x-gzip;")

	kde4_add_plugin(kerfuffle_libgz ${kerfuffle_libgz_SRCS})

	target_link_libraries(kerfuffle_libgz ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} kerfuffle )

	install(TARGETS kerfuffle_libgz DESTINATION ${PLUGIN_INSTALL_DIR} )

	install( FILES kerfuffle_libgz.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
endif (ZLIB_FOUND)

#
# Bzip2 files
#
macro_optional_find_package(BZip2)

macro_log_feature(BZIP2_FOUND "BZip2" "A high-quality data compressor" "http://www.bzip.org" FALSE "" "Required for the .bz2 format support in Ark")

if (BZIP2_FOUND)
	set(kerfuffle_libbz2_SRCS bz2plugin.cpp ${kerfuffle_singlefile_SRCS})
	set(SUPPORTED_LIBSINGLEFILE_MIMETYPES "${SUPPORTED_LIBSINGLEFILE_MIMETYPES}application/x-bzip;application/x-bzip2;")

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
	set(SUPPORTED_LIBSINGLEFILE_MIMETYPES "${SUPPORTED_LIBSINGLEFILE_MIMETYPES}application/x-lzma;application/x-xz;")

	kde4_add_plugin(kerfuffle_libxz ${kerfuffle_libxz_SRCS})

	target_link_libraries(kerfuffle_libxz ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} kerfuffle )

	install(TARGETS kerfuffle_libxz DESTINATION ${PLUGIN_INSTALL_DIR} )

	install( FILES kerfuffle_libxz.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
endif (LIBLZMA_FOUND)

set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${SUPPORTED_LIBSINGLEFILE_MIMETYPES}" PARENT_SCOPE)
