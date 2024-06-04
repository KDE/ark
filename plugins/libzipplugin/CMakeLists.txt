set(INSTALLED_LIBZIP_PLUGINS "")

# not yet supported on freebsd
try_compile(HAVE_CHRONO_CAST ${CMAKE_CURRENT_BINARY_DIR} SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/.test_chrono_cast.cpp)

set(kerfuffle_libzip_SRCS libzipplugin.cpp ark_debug.cpp)

ecm_qt_declare_logging_category(kerfuffle_libzip_SRCS
                                HEADER ark_debug.h
                                IDENTIFIER ARK_LOG
                                CATEGORY_NAME ark.libzip DESCRIPTION "Ark Infozip plugin" EXPORT ARK)

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

kerfuffle_parse_json_mimetypes(${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_libzip.json)
set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${PLUGIN_SUPPORTED_MIMETYPES}" PARENT_SCOPE)

kerfuffle_add_plugin(kerfuffle_libzip ${kerfuffle_libzip_SRCS})

if (HAVE_CHRONO_CAST)
    target_compile_definitions(kerfuffle_libzip PRIVATE HAVE_CHRONO_CAST=1)
else()
    target_compile_definitions(kerfuffle_libzip PRIVATE HAVE_CHRONO_CAST=0)
endif()

target_link_libraries(kerfuffle_libzip KF6::KIOCore LibZip::LibZip ZLIB::ZLIB)

set(INSTALLED_LIBZIP_PLUGINS "${INSTALLED_LIBZIP_PLUGINS}kerfuffle_libzip;")

set(INSTALLED_KERFUFFLE_PLUGINS "${INSTALLED_KERFUFFLE_PLUGINS}${INSTALLED_LIBZIP_PLUGINS}" PARENT_SCOPE)
