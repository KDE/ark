########### next target ###############

set(SUPPORTED_CLI7Z_MIMETYPES "application/x-7z-compressed;application/zip;")

set(kerfuffle_cli7z_SRCS cliplugin.cpp)

ecm_qt_declare_logging_category(kerfuffle_cli7z_SRCS
                                HEADER ark_debug.h
                                IDENTIFIER ARK
                                CATEGORY_NAME ark.cli7z DESCRIPTION "Ark p7zip" EXPORT ARK)

# NOTE: the first double-quotes of the first mime and the last
# double-quotes of the last mime must NOT be escaped.
set(SUPPORTED_MIMETYPES
    "application/x-7z-compressed\",
    \"application/zip")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_cli7z.json.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_cli7z.json)

kerfuffle_add_plugin(kerfuffle_cli7z ${kerfuffle_cli7z_SRCS})

set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${SUPPORTED_CLI7Z_MIMETYPES}" PARENT_SCOPE)
set(INSTALLED_KERFUFFLE_PLUGINS "${INSTALLED_KERFUFFLE_PLUGINS}kerfuffle_cli7z;" PARENT_SCOPE)

find_program(P7ZIP 7z)
if(P7ZIP)
    message(STATUS "Found p7zip executable: ${P7ZIP}")
else()
    message(WARNING "Could not find the 7z executable. Ark won't be able to handle the 7z archive format.")
endif()
