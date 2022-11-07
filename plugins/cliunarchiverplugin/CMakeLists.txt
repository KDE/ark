########### next target ###############

set(kerfuffle_cliunarchiver_SRCS cliplugin.cpp)

ecm_qt_declare_logging_category(kerfuffle_cliunarchiver_SRCS
                                HEADER ark_debug.h
                                IDENTIFIER ARK
                                CATEGORY_NAME ark.cliunarchiver DESCRIPTION "Ark The Unarchiver plugin" EXPORT ARK)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_cliunarchiver.json.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_cliunarchiver.json)

kerfuffle_parse_json_mimetypes(${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_cliunarchiver.json)
set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${PLUGIN_SUPPORTED_MIMETYPES}" PARENT_SCOPE)

kerfuffle_add_plugin(kerfuffle_cliunarchiver ${kerfuffle_cliunarchiver_SRCS})

# This plugin has a catch() block
kde_target_enable_exceptions(kerfuffle_cliunarchiver PRIVATE)

set(INSTALLED_KERFUFFLE_PLUGINS "${INSTALLED_KERFUFFLE_PLUGINS}kerfuffle_cliunarchiver;" PARENT_SCOPE)

find_program(UNAR unar)
if(UNAR)
    message(STATUS "Found unar executable: ${UNAR}")
else()
    message(WARNING "Could not find the unar executable. Ark requires unar or unrar to extract RAR archives.")
endif()
