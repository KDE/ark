########### next target ###############

set(kerfuffle_cliarj_SRCS cliplugin.cpp cliplugin.h)

ecm_qt_declare_logging_category(kerfuffle_cliarj_SRCS
                                HEADER ark_debug.h
                                IDENTIFIER ARK_LOG
                                CATEGORY_NAME ark.cliarj DESCRIPTION "Ark Arj" EXPORT ARK)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_cliarj.json.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_cliarj.json)

kerfuffle_parse_json_mimetypes(${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_cliarj.json)
set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${PLUGIN_SUPPORTED_MIMETYPES}" PARENT_SCOPE)

kerfuffle_add_plugin(kerfuffle_cliarj ${kerfuffle_cliarj_SRCS})

set(INSTALLED_KERFUFFLE_PLUGINS "${INSTALLED_KERFUFFLE_PLUGINS}kerfuffle_cliarj;" PARENT_SCOPE)

find_program(ARJ arj)
if(ARJ)
    message(STATUS "Found arj executable: ${ARJ}")
else()
    message(WARNING "Could not find the arj executable. Ark won't be able to handle the arj archive format.")
endif()
