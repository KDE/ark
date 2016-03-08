########### next target ###############

set(SUPPORTED_KARCHIVE_MIMETYPES "application/zip;application/x-zip-compressed;application/x-tar;application/x-compressed-tar;application/x-bzip-compressed-tar;application/x-xz-compressed-tar;")

set(kerfuffle_karchive_SRCS karchiveplugin.cpp)

ecm_qt_declare_logging_category(kerfuffle_karchive_SRCS
                                HEADER ark_debug.h
                                IDENTIFIER ARK
                                CATEGORY_NAME ark.karchive)

add_library(kerfuffle_karchive MODULE ${kerfuffle_karchive_SRCS})

target_link_libraries(kerfuffle_karchive KF5::Archive KF5::KIOCore kerfuffle )

configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_karchive.desktop.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_karchive.desktop
)

kcoreaddons_desktop_to_json(kerfuffle_karchive ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_karchive.desktop)

########### install files ###############

install(TARGETS kerfuffle_karchive  DESTINATION ${PLUGIN_INSTALL_DIR}/kerfuffle)

set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${SUPPORTED_KARCHIVE_MIMETYPES}" PARENT_SCOPE)
