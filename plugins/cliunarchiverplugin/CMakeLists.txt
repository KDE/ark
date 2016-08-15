########### next target ###############

set(SUPPORTED_CLIUNARCHIVER_MIMETYPES "application/x-rar;")

set(kerfuffle_cliunarchiver_SRCS cliplugin.cpp)

ecm_qt_declare_logging_category(kerfuffle_cliunarchiver_SRCS
                                HEADER ark_debug.h
                                IDENTIFIER ARK
                                CATEGORY_NAME ark.cliunarchiver)

set(SUPPORTED_MIMETYPES "application/x-rar")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_cliunarchiver.json.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_cliunarchiver.json)

kerfuffle_add_plugin(kerfuffle_cliunarchiver ${kerfuffle_cliunarchiver_SRCS})

set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${SUPPORTED_CLIUNARCHIVER_MIMETYPES}"
PARENT_SCOPE)
set(INSTALLED_KERFUFFLE_PLUGINS "${INSTALLED_KERFUFFLE_PLUGINS}kerfuffle_cliunarchiver;" PARENT_SCOPE)
