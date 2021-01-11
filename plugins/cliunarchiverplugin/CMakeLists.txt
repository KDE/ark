########### next target ###############

# TODO: drop application/x-rar alias once distributions ship shared-mime-info 1.7
set(SUPPORTED_CLIUNARCHIVER_MIMETYPES "application/vnd.rar;application/x-rar;application/x-lha;")

set(kerfuffle_cliunarchiver_SRCS cliplugin.cpp)

ecm_qt_declare_logging_category(kerfuffle_cliunarchiver_SRCS
                                HEADER ark_debug.h
                                IDENTIFIER ARK
                                CATEGORY_NAME ark.cliunarchiver DESCRIPTION "Ark The Unarchiver plugin" EXPORT ARK)

# NOTE: the first double-quotes of the first mime and the last
# double-quotes of the last mime must NOT be escaped.
set(SUPPORTED_MIMETYPES
    "application/vnd.rar\",
    \"application/x-rar\",
    \"application/x-lha")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_cliunarchiver.json.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_cliunarchiver.json)

kerfuffle_add_plugin(kerfuffle_cliunarchiver ${kerfuffle_cliunarchiver_SRCS})

# This plugin has a catch() block
kde_target_enable_exceptions(kerfuffle_cliunarchiver PRIVATE)

set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${SUPPORTED_CLIUNARCHIVER_MIMETYPES}"
PARENT_SCOPE)
set(INSTALLED_KERFUFFLE_PLUGINS "${INSTALLED_KERFUFFLE_PLUGINS}kerfuffle_cliunarchiver;" PARENT_SCOPE)

find_program(UNAR unar)
if(UNAR)
    message(STATUS "Found unar executable: ${UNAR}")
else()
    message(WARNING "Could not find the unar executable. Ark requires unar or unrar to extract RAR archives.")
endif()
