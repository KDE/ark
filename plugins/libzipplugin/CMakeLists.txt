set(SUPPORTED_LIBZIP_MIMETYPES "application/zip;")

set(INSTALLED_LIBZIP_PLUGINS "")

set(kerfuffle_libzip_SRCS libzipplugin.cpp ark_debug.cpp)

ecm_qt_declare_logging_category(kerfuffle_libzip_SRCS
                                HEADER ark_debug.h
                                IDENTIFIER ARK
                                CATEGORY_NAME ark.libzip DESCRIPTION "Ark Infozip plugin" EXPORT ARK)

# NOTE: the first double-quotes of the first mime and the last
# double-quotes of the last mime must NOT be escaped.
set(SUPPORTED_MIMETYPES "application/zip")

if(LibZip_VERSION VERSION_GREATER_EQUAL "1.8.0")
    set(ZIP_CM_ZSTD_JSON_LINE "\"Zstd\": \"Zstd\",")
    set(ZIP_CM_LZMA_JSON_LINE "\"LZMA\": \"LZMA\",")
    set(ZIP_CM_XZ_JSON_LINE "\"XZ\": \"XZ\",")
endif()

find_package(ZLIB REQUIRED)
set_package_properties(ZLIB PROPERTIES
                       URL "https://www.zlib.net/"
                       DESCRIPTION "The Zlib compression library"
                       PURPOSE "Required for integrity check in libzip plugin")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_libzip.json.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_libzip.json)

kerfuffle_add_plugin(kerfuffle_libzip ${kerfuffle_libzip_SRCS})

target_link_libraries(kerfuffle_libzip KF5::KIOCore LibZip::LibZip ZLIB::ZLIB)

set(INSTALLED_LIBZIP_PLUGINS "${INSTALLED_LIBZIP_PLUGINS}kerfuffle_libzip;")

set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${SUPPORTED_LIBZIP_MIMETYPES}" PARENT_SCOPE)
set(INSTALLED_KERFUFFLE_PLUGINS "${INSTALLED_KERFUFFLE_PLUGINS}${INSTALLED_LIBZIP_PLUGINS}" PARENT_SCOPE)
