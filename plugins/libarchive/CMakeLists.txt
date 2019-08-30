include_directories(${LibArchive_INCLUDE_DIRS})

if(NOT LibArchive_VERSION VERSION_LESS "3.3.3")
  set(ENABLE_ZSTD_SUPPORT ON BOOL)
endif()
if(NOT LibArchive_VERSION VERSION_LESS "3.3.3")
  set(ENABLE_BROTLI_SUPPORT ON BOOL)
endif()

########### next target ###############
set(SUPPORTED_LIBARCHIVE_READWRITE_MIMETYPES "application/x-tar;application/x-compressed-tar;application/x-bzip-compressed-tar;application/x-tarz;application/x-xz-compressed-tar;")
set(SUPPORTED_LIBARCHIVE_READWRITE_MIMETYPES "${SUPPORTED_LIBARCHIVE_READWRITE_MIMETYPES}application/x-lzma-compressed-tar;application/x-lzip-compressed-tar;application/x-tzo;application/x-lrzip-compressed-tar;application/x-lz4-compressed-tar;")
set(SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES "application/vnd.debian.binary-package;application/x-deb;application/x-cd-image;application/x-bcpio;application/x-cpio;application/x-cpio-compressed;application/x-sv4cpio;application/x-sv4crc;")
set(SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES "${SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES}application/x-rpm;application/x-source-rpm;application/vnd.ms-cab-compressed;application/x-xar;application/x-iso9660-appimage;application/x-archive;")

if(ENABLE_ZSTD_SUPPORT)
    set(SUPPORTED_LIBARCHIVE_READWRITE_MIMETYPES "${SUPPORTED_LIBARCHIVE_READWRITE_MIMETYPES}application/x-zstd-compressed-tar;")
endif()
if(ENABLE_BROTLI_SUPPORT)
    set(SUPPORTED_LIBARCHIVE_READWRITE_MIMETYPES "${SUPPORTED_LIBARCHIVE_READWRITE_MIMETYPES}application/x-brotli-compressed-tar;")
endif()

set(INSTALLED_LIBARCHIVE_PLUGINS "")

set(kerfuffle_libarchive_readonly_SRCS libarchiveplugin.cpp readonlylibarchiveplugin.cpp ark_debug.cpp)
set(kerfuffle_libarchive_readwrite_SRCS libarchiveplugin.cpp readwritelibarchiveplugin.cpp ark_debug.cpp)
set(kerfuffle_libarchive_SRCS ${kerfuffle_libarchive_readonly_SRCS} readwritelibarchiveplugin.cpp)

ecm_qt_declare_logging_category(kerfuffle_libarchive_SRCS
                                HEADER ark_debug.h
                                IDENTIFIER ARK
                                CATEGORY_NAME ark.libarchive)

# NOTE: the first double-quotes of the first mime and the last
# double-quotes of the last mime must NOT be escaped.
set(SUPPORTED_READONLY_MIMETYPES
    "application/x-deb\",
    \"application/x-cd-image\",
    \"application/x-bcpio\",
    \"application/x-cpio\",
    \"application/x-cpio-compressed\",
    \"application/x-sv4cpio\",
    \"application/x-sv4crc\",
    \"application/x-rpm\",
    \"application/x-source-rpm\",
    \"application/vnd.debian.binary-package\",
    \"application/vnd.ms-cab-compressed\",
    \"application/x-xar\",
    \"application/x-iso9660-appimage\",
    \"application/x-archive")

# NOTE: the first double-quotes of the first mime and the last
# double-quotes of the last mime must NOT be escaped.
set(SUPPORTED_READWRITE_MIMETYPES
    "application/x-tar\",
    \"application/x-compressed-tar\",
    \"application/x-bzip-compressed-tar\",
    \"application/x-tarz\",
    \"application/x-xz-compressed-tar\",
    \"application/x-lzma-compressed-tar\",
    \"application/x-lzip-compressed-tar\",
    \"application/x-tzo\",
    \"application/x-lrzip-compressed-tar\",
    \"application/x-lz4-compressed-tar")

if(ENABLE_ZSTD_SUPPORT)
    set(SUPPORTED_READWRITE_MIMETYPES
        "${SUPPORTED_READWRITE_MIMETYPES}\",
        \"application/x-zstd-compressed-tar")
endif()
if(ENABLE_BROTLI_SUPPORT)
    set(SUPPORTED_READWRITE_MIMETYPES
        "${SUPPORTED_READWRITE_MIMETYPES}\",
        \"application/x-brotli-compressed-tar")
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_libarchive_readonly.json.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_libarchive_readonly.json)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_libarchive.json.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_libarchive.json)

kerfuffle_add_plugin(kerfuffle_libarchive_readonly ${kerfuffle_libarchive_readonly_SRCS})
kerfuffle_add_plugin(kerfuffle_libarchive ${kerfuffle_libarchive_readwrite_SRCS})

if(ENABLE_ZSTD_SUPPORT)
    target_compile_definitions(kerfuffle_libarchive PRIVATE -DHAVE_ZSTD_SUPPORT)
endif()
if(ENABLE_BROTLI_SUPPORT)
    target_compile_definitions(kerfuffle_libarchive PRIVATE -DHAVE_BROTLI_SUPPORT)
endif()

target_link_libraries(kerfuffle_libarchive_readonly ${LibArchive_LIBRARIES})
target_link_libraries(kerfuffle_libarchive ${LibArchive_LIBRARIES})

set(INSTALLED_LIBARCHIVE_PLUGINS "${INSTALLED_LIBARCHIVE_PLUGINS}kerfuffle_libarchive_readonly;")
set(INSTALLED_LIBARCHIVE_PLUGINS "${INSTALLED_LIBARCHIVE_PLUGINS}kerfuffle_libarchive;")

set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${SUPPORTED_LIBARCHIVE_READWRITE_MIMETYPES}${SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES}" PARENT_SCOPE)
set(INSTALLED_KERFUFFLE_PLUGINS "${INSTALLED_KERFUFFLE_PLUGINS}${INSTALLED_LIBARCHIVE_PLUGINS}" PARENT_SCOPE)

find_program(LRZIP lrzip)
if(LRZIP)
    message(STATUS "Found lrzip executable: ${LRZIP}")
else()
    message(WARNING "Could not find the lrzip executable. Ark requires lrzip to handle the tar.lrz archive format.")
endif()

find_program(LZOP lzop)
if(LZOP)
    message(STATUS "Found lzop executable: ${LZOP}")
else()
    message(WARNING "Could not find the lzop executable. Ark requires lzop to handle the tar.lzo archive format if libarchive >= 3.3 has been compiled without liblzo2 support.")
endif()

find_program(ZSTD zstd)
if(ZSTD)
    message(STATUS "Found zstd executable: ${ZSTD}")
else()
    message(WARNING "Could not find the zstd executable. Ark requires zstd to handle the tar.zst archive format if libarchive >= 3.3.3 has been compiled without libzstd support.")
endif()

find_program(BROTLI brotli)
if(BROTLI)
    message(STATUS "Found brotli executable: ${BROTLI}")
else()
    message(WARNING "Could not find the brotli executable. Ark requires brotli to handle the tar.br archive format if libarchive >= 3.4.0 has been compiled without libbrotli support.")
endif()
