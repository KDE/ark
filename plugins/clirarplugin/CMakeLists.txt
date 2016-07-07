########### next target ###############

set(SUPPORTED_CLIRAR_MIMETYPES "application/x-rar;")

set(kerfuffle_clirar_SRCS cliplugin.cpp)

ecm_qt_declare_logging_category(kerfuffle_clirar_SRCS
                                HEADER ark_debug.h
                                IDENTIFIER ARK
                                CATEGORY_NAME ark.clirar)

set(SUPPORTED_MIMETYPES "application/x-rar")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_clirar.json.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_clirar.json)

kerfuffle_add_plugin(kerfuffle_clirar ${kerfuffle_clirar_SRCS})

set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${SUPPORTED_CLIRAR_MIMETYPES}" PARENT_SCOPE)
set(INSTALLED_KERFUFFLE_PLUGINS "${INSTALLED_KERFUFFLE_PLUGINS}kerfuffle_clirar;" PARENT_SCOPE)
