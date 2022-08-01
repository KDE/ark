########### next target ###############

set(SUPPORTED_CLIARJ_MIMETYPES "application/x-arj;application/arj;")

set(kerfuffle_cliarj_SRCS cliplugin.cpp)

ecm_qt_declare_logging_category(kerfuffle_cliarj_SRCS
                                HEADER ark_debug.h
                                IDENTIFIER ARK
                                CATEGORY_NAME ark.cliarj DESCRIPTION "Ark Arj" EXPORT ARK)

# NOTE: the first double-quotes of the first mime and the last
# double-quotes of the last mime must NOT be escaped.
set(SUPPORTED_MIMETYPES
    "application/x-arj\",
    \"application/arj")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_cliarj.json.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_cliarj.json)

kerfuffle_add_plugin(kerfuffle_cliarj ${kerfuffle_cliarj_SRCS})

set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${SUPPORTED_CLIARJ_MIMETYPES}" PARENT_SCOPE)
set(INSTALLED_KERFUFFLE_PLUGINS "${INSTALLED_KERFUFFLE_PLUGINS}kerfuffle_cliarj;" PARENT_SCOPE)

find_program(ARJ arj)
if(ARJ)
    message(STATUS "Found arj executable: ${ARJ}")
else()
    message(WARNING "Could not find the arj executable. Ark won't be able to handle the arj archive format.")
endif()
