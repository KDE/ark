########### next target ###############

set(SUPPORTED_CLIRAR_MIMETYPES "application/x-rar;")

set(kerfuffle_clirar_SRCS cliplugin.cpp)

ecm_qt_declare_logging_category(kerfuffle_clirar_SRCS
                                HEADER ark_debug.h
                                IDENTIFIER ARK
                                CATEGORY_NAME ark.clirar)

add_library(kerfuffle_clirar MODULE ${kerfuffle_clirar_SRCS})

target_link_libraries(kerfuffle_clirar kerfuffle)

install(TARGETS kerfuffle_clirar DESTINATION ${KDE_INSTALL_PLUGINDIR}/kerfuffle)

set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${SUPPORTED_CLIRAR_MIMETYPES}" PARENT_SCOPE)
set(INSTALLED_KERFUFFLE_PLUGINS "${INSTALLED_KERFUFFLE_PLUGINS}kerfuffle_clirar;" PARENT_SCOPE)
