########### next target ###############

set(SUPPORTED_CLIUNARCHIVER_MIMETYPES "application/x-rar;")

set(kerfuffle_cliunarchiver_SRCS cliplugin.cpp)

ecm_qt_declare_logging_category(kerfuffle_cliunarchiver_SRCS
                                HEADER ark_debug.h
                                IDENTIFIER ARK
                                CATEGORY_NAME ark.cliunarchiver)

add_library(kerfuffle_cliunarchiver MODULE ${kerfuffle_cliunarchiver_SRCS})

target_link_libraries(kerfuffle_cliunarchiver kerfuffle)

configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_cliunarchiver.desktop.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_cliunarchiver.desktop
)

########### install files ###############

install(TARGETS kerfuffle_cliunarchiver DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_cliunarchiver.desktop DESTINATION ${SERVICES_INSTALL_DIR})

set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${SUPPORTED_CLIUNARCHIVER_MIMETYPES}"
PARENT_SCOPE)
