########### next target ###############

set(SUPPORTED_CLI7Z_MIMETYPES "application/x-7z-compressed;application/zip;")

set(kerfuffle_cli7z_SRCS cliplugin.cpp)

ecm_qt_declare_logging_category(kerfuffle_cli7z_SRCS
                                HEADER ark_debug.h
                                IDENTIFIER ARK
                                CATEGORY_NAME ark.cli7z)

add_library(kerfuffle_cli7z MODULE ${kerfuffle_cli7z_SRCS})

target_link_libraries(kerfuffle_cli7z kerfuffle)

########### install files ###############

install(TARGETS kerfuffle_cli7z  DESTINATION ${KDE_INSTALL_PLUGINDIR}/kerfuffle)

set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${SUPPORTED_CLI7Z_MIMETYPES}" PARENT_SCOPE)
