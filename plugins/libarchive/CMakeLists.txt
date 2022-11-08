include_directories(${LibArchive_INCLUDE_DIRS})

########### next target ###############

# NOTE: These are the mimetypes for "single-file" archives. They must be defined in the JSON metadata together with the "normal" mimetypes.
# However they need to be duplicated here because we need to pass them as C++ define to the plugin (see LIBARCHIVE_RAW_MIMETYPES define below).
set(SUPPORTED_LIBARCHIVE_RAW_MIMETYPES "application/x-compress;application/gzip;application/x-bzip;application/zlib;application/zstd;application/x-lzma;application/x-xz;application/x-lz4;application/x-lzip;application/x-lrzip;application/x-lzop;")

set(INSTALLED_LIBARCHIVE_PLUGINS "")

set(kerfuffle_libarchive_readonly_SRCS libarchiveplugin.cpp readonlylibarchiveplugin.cpp ark_debug.cpp)
set(kerfuffle_libarchive_readwrite_SRCS libarchiveplugin.cpp readwritelibarchiveplugin.cpp ark_debug.cpp)
set(kerfuffle_libarchive_SRCS ${kerfuffle_libarchive_readonly_SRCS} readwritelibarchiveplugin.cpp)

ecm_qt_declare_logging_category(kerfuffle_libarchive_SRCS
                                HEADER ark_debug.h
                                IDENTIFIER ARK
                                CATEGORY_NAME ark.libarchive DESCRIPTION "Ark Libarchive plugin" EXPORT ARK)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_libarchive_readonly.json.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_libarchive_readonly.json)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_libarchive.json.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_libarchive.json)

kerfuffle_parse_json_mimetypes(${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_libarchive_readonly.json)
set(SUPPORTED_LIBARCHIVE_MIMETYPES "${SUPPORTED_LIBARCHIVE_MIMETYPES}${PLUGIN_SUPPORTED_MIMETYPES}")

kerfuffle_parse_json_mimetypes(${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_libarchive.json)
set(SUPPORTED_LIBARCHIVE_MIMETYPES "${SUPPORTED_LIBARCHIVE_MIMETYPES}${PLUGIN_SUPPORTED_MIMETYPES}")

set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${SUPPORTED_LIBARCHIVE_MIMETYPES}" PARENT_SCOPE)

kerfuffle_add_plugin(kerfuffle_libarchive_readonly ${kerfuffle_libarchive_readonly_SRCS})
kerfuffle_add_plugin(kerfuffle_libarchive ${kerfuffle_libarchive_readwrite_SRCS})

# Concat the list of raw mimetypes in a single string, so that we can pass it as C++ define to the plugin.
# NOTE: cannot use ";" as separator since it breaks cmake badly.
list(JOIN SUPPORTED_LIBARCHIVE_RAW_MIMETYPES ":" RAW_MIMETYPES_CONCAT)
target_compile_definitions(kerfuffle_libarchive_readonly PRIVATE -DLIBARCHIVE_RAW_MIMETYPES="${RAW_MIMETYPES_CONCAT}")

target_link_libraries(kerfuffle_libarchive_readonly ${LibArchive_LIBRARIES})
target_link_libraries(kerfuffle_libarchive ${LibArchive_LIBRARIES})

set(INSTALLED_LIBARCHIVE_PLUGINS "${INSTALLED_LIBARCHIVE_PLUGINS}kerfuffle_libarchive_readonly;")
set(INSTALLED_LIBARCHIVE_PLUGINS "${INSTALLED_LIBARCHIVE_PLUGINS}kerfuffle_libarchive;")

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
