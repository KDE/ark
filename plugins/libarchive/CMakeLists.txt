include_directories(${LibArchive_INCLUDE_DIRS})

########### next target ###############
set(SUPPORTED_LIBARCHIVE_READWRITE_MIMETYPES "application/x-tar;application/x-compressed-tar;application/x-bzip-compressed-tar;application/x-tarz;application/x-xz-compressed-tar;")
set(SUPPORTED_LIBARCHIVE_READWRITE_MIMETYPES "${SUPPORTED_LIBARCHIVE_READWRITE_MIMETYPES}application/x-lzma-compressed-tar;application/x-lzip-compressed-tar;application/x-tzo;application/x-lrzip-compressed-tar;")
set(SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES "application/vnd.debian.binary-package;application/x-deb;application/x-cd-image;application/x-bcpio;application/x-cpio;application/x-cpio-compressed;application/x-sv4cpio;application/x-sv4crc;")
set(SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES "${SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES}application/x-rpm;application/x-source-rpm;application/vnd.ms-cab-compressed;application/x-xar;")

if(LibArchive_VERSION VERSION_EQUAL "3.2.0" OR
   LibArchive_VERSION VERSION_GREATER "3.2.0")
  set(SUPPORTED_LIBARCHIVE_READWRITE_MIMETYPES "${SUPPORTED_LIBARCHIVE_READWRITE_MIMETYPES}application/x-lz4-compressed-tar;")
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
    \"application/x-xar")

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
    \"application/x-lrzip-compressed-tar")

if(LibArchive_VERSION VERSION_EQUAL "3.2.0" OR
   LibArchive_VERSION VERSION_GREATER "3.2.0")
  set(SUPPORTED_READWRITE_MIMETYPES
      "${SUPPORTED_READWRITE_MIMETYPES}\",
      \"application/x-lz4-compressed-tar")
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_libarchive_readonly.json.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_libarchive_readonly.json)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_libarchive.json.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_libarchive.json)

kerfuffle_add_plugin(kerfuffle_libarchive_readonly ${kerfuffle_libarchive_readonly_SRCS})
kerfuffle_add_plugin(kerfuffle_libarchive ${kerfuffle_libarchive_readwrite_SRCS})

if(LibArchive_VERSION VERSION_EQUAL "3.2.0" OR
   LibArchive_VERSION VERSION_GREATER "3.2.0")
  target_compile_definitions(kerfuffle_libarchive PRIVATE -DHAVE_LIBARCHIVE_3_2_0)
endif()

target_link_libraries(kerfuffle_libarchive_readonly ${LibArchive_LIBRARIES})
target_link_libraries(kerfuffle_libarchive ${LibArchive_LIBRARIES})

set(INSTALLED_LIBARCHIVE_PLUGINS "${INSTALLED_LIBARCHIVE_PLUGINS}kerfuffle_libarchive_readonly;")
set(INSTALLED_LIBARCHIVE_PLUGINS "${INSTALLED_LIBARCHIVE_PLUGINS}kerfuffle_libarchive;")

set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${SUPPORTED_LIBARCHIVE_READWRITE_MIMETYPES}${SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES}" PARENT_SCOPE)
set(INSTALLED_KERFUFFLE_PLUGINS "${INSTALLED_KERFUFFLE_PLUGINS}${INSTALLED_LIBARCHIVE_PLUGINS}" PARENT_SCOPE)
