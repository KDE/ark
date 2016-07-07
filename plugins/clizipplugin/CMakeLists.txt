########### next target ###############

set(SUPPORTED_CLIZIP_MIMETYPES "application/x-java-archive;application/zip;")

set(kerfuffle_clizip_SRCS cliplugin.cpp)

ecm_qt_declare_logging_category(kerfuffle_clizip_SRCS
                                HEADER ark_debug.h
                                IDENTIFIER ARK
                                CATEGORY_NAME ark.clizip)

# NOTE: the first double-quotes of the first mime and the last
# double-quotes of the last mime must NOT be escaped.
set(SUPPORTED_MIMETYPES
    "application/x-java-archive\",
    \"application/zip")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_clizip.json.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_clizip.json)

kerfuffle_add_plugin(kerfuffle_clizip ${kerfuffle_clizip_SRCS})

set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${SUPPORTED_CLIZIP_MIMETYPES}" PARENT_SCOPE)
set(INSTALLED_KERFUFFLE_PLUGINS "${INSTALLED_KERFUFFLE_PLUGINS}kerfuffle_clizip;" PARENT_SCOPE)
