set(kerfuffle_singlefile_SRCS singlefileplugin.cpp)

ecm_qt_declare_logging_category(kerfuffle_singlefile_SRCS
                                HEADER ark_debug.h
                                IDENTIFIER ARK
                                CATEGORY_NAME ark.singlefile)

set(SUPPORTED_LIBSINGLEFILE_MIMETYPES "")
set(INSTALLED_LIBSINGLEFILE_PLUGINS "")

# This MIME type was originally set in ark.desktop but is does not
# seem to be supported anywhere.  Assuming that, if it were supported,
# it would be here.
set(SUPPORTED_LIBSINGLEFILE_MIMETYPES "${SUPPORTED_LIBSINGLEFILE_MIMETYPES}application/x-compress;")

#
# GZip files
#
find_package(ZLIB)
set_package_properties(ZLIB PROPERTIES
                       URL "https://www.zlib.net/"
                       DESCRIPTION "The Zlib compression library"
                       PURPOSE "Required for .gz format support in Ark")

if (ZLIB_FOUND)
    set(kerfuffle_libgz_SRCS gzplugin.cpp ${kerfuffle_singlefile_SRCS})
    set(SUPPORTED_LIBSINGLEFILE_MIMETYPES "${SUPPORTED_LIBSINGLEFILE_MIMETYPES}application/gzip;")

    set(SUPPORTED_MIMETYPES "application/gzip")

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_libgz.json.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_libgz.json)

    kerfuffle_add_plugin(kerfuffle_libgz ${kerfuffle_libgz_SRCS})
    target_link_libraries(kerfuffle_libgz KF5::Archive)

    set(INSTALLED_LIBSINGLEFILE_PLUGINS "${INSTALLED_LIBSINGLEFILE_PLUGINS}kerfuffle_libgz;")
endif (ZLIB_FOUND)

#
# Bzip2 files
#
find_package(BZip2)
set_package_properties(BZip2 PROPERTIES
                       URL "https://sourceware.org/bzip2/"
                       DESCRIPTION "The Bzip2 compression library"
                       PURPOSE "Required for .bz2 format support in Ark")

if (BZIP2_FOUND)
	set(kerfuffle_libbz2_SRCS bz2plugin.cpp ${kerfuffle_singlefile_SRCS})
	set(SUPPORTED_LIBSINGLEFILE_MIMETYPES "${SUPPORTED_LIBSINGLEFILE_MIMETYPES}application/x-bzip;")

    set(SUPPORTED_MIMETYPES "application/x-bzip")

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_libbz2.json.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_libbz2.json)

    kerfuffle_add_plugin(kerfuffle_libbz2 ${kerfuffle_libbz2_SRCS})
    target_link_libraries(kerfuffle_libbz2 KF5::Archive)

    set(INSTALLED_LIBSINGLEFILE_PLUGINS "${INSTALLED_LIBSINGLEFILE_PLUGINS}kerfuffle_libbz2;")
endif (BZIP2_FOUND)

#
# LZMA files
#
find_package(LibLZMA)
set_package_properties(LibLZMA PROPERTIES
                       URL "https://tukaani.org/xz/"
                       DESCRIPTION "A LZMA compression library"
                       PURPOSE "Required for .xz and .lzma format support in Ark")

if (LIBLZMA_FOUND)
	set(kerfuffle_libxz_SRCS xzplugin.cpp ${kerfuffle_singlefile_SRCS})
	set(SUPPORTED_LIBSINGLEFILE_MIMETYPES "${SUPPORTED_LIBSINGLEFILE_MIMETYPES}application/x-lzma;application/x-xz;")

    # NOTE: the first double-quotes of the first mime and the last
    # double-quotes of the last mime must NOT be escaped.
    set(SUPPORTED_MIMETYPES
        "application/x-lzma\",
        \"application/x-xz")

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_libxz.json.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_libxz.json)

    kerfuffle_add_plugin(kerfuffle_libxz ${kerfuffle_libxz_SRCS})
    target_link_libraries(kerfuffle_libxz KF5::Archive)

    set(INSTALLED_LIBSINGLEFILE_PLUGINS "${INSTALLED_LIBSINGLEFILE_PLUGINS}kerfuffle_libxz;")
endif (LIBLZMA_FOUND)

set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${SUPPORTED_LIBSINGLEFILE_MIMETYPES}" PARENT_SCOPE)
set(INSTALLED_KERFUFFLE_PLUGINS "${INSTALLED_KERFUFFLE_PLUGINS}${INSTALLED_LIBSINGLEFILE_PLUGINS}" PARENT_SCOPE)
