########### next target ###############

set(kerfuffle_cli7z_SRCS cliplugin.cpp cliplugin.h)

ecm_qt_declare_logging_category(kerfuffle_cli7z_SRCS
                                HEADER ark_debug.h
                                IDENTIFIER ARK_LOG
                                CATEGORY_NAME ark.cli7z DESCRIPTION "Ark 7z" EXPORT ARK)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_cli7z.json.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_cli7z.json)

kerfuffle_parse_json_mimetypes(${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_cli7z.json)
set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${PLUGIN_SUPPORTED_MIMETYPES}" PARENT_SCOPE)

kerfuffle_add_plugin(kerfuffle_cli7z ${kerfuffle_cli7z_SRCS})

set(INSTALLED_KERFUFFLE_PLUGINS "${INSTALLED_KERFUFFLE_PLUGINS}kerfuffle_cli7z;" PARENT_SCOPE)

find_program(7ZIP 7z)
if(7ZIP)
    message(STATUS "Found 7-Zip executable: ${7ZIP}")
else()
    message(WARNING "Could not find the 7z executable. Ark will only provide limited support for the 7z archive format.")
endif()
