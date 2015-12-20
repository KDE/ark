########### next target ###############

set(SUPPORTED_CLIUNARCHIVER_MIMETYPES "application/x-7z-compressed;application/zip;")
set(ENCRYPT_ENTRIES_MIMETYPES "application/x-7z-compressed,application/zip")
set(ENCRYPT_HEADER_MIMETYPES "application/x-7z-compressed")

set(kerfuffle_cliunarchiver_SRCS cliplugin.cpp)

ecm_qt_declare_logging_category(kerfuffle_cliunarchiver_SRCS
                                HEADER ark_debug.h
                                IDENTIFIER ARK
                                CATEGORY_NAME ark.cliunarchiver)

add_library(kerfuffle_cliunarchiver MODULE ${kerfuffle_cliunarchiver_SRCS})

target_link_libraries(kerfuffle_cliunarchiver kerfuffle)

########### install files ###############

install(TARGETS kerfuffle_cliunarchiver DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kerfuffle_cliunarchiver.desktop DESTINATION ${SERVICES_INSTALL_DIR})

set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${SUPPORTED_CLIUNARCHIVER_MIMETYPES}" PARENT_SCOPE)
