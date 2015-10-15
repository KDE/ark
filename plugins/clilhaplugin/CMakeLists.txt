########### next target ###############

set(SUPPORTED_CLILHA_MIMETYPES "application/x-lha;")

set(kerfuffle_clilha_SRCS cliplugin.cpp)

ecm_qt_declare_logging_category(kerfuffle_clilha_SRCS
                                HEADER ark_debug.h
                                IDENTIFIER ARK
                                CATEGORY_NAME ark.clilha)

add_library(kerfuffle_clilha MODULE ${kerfuffle_cli_SRCS})

target_link_libraries(kerfuffle_clilha kerfuffle )

configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_clilha.desktop.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_clilha.desktop
)

########### install files ###############

install(TARGETS kerfuffle_clilha  DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_clilha.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${SUPPORTED_CLILHA_MIMETYPES}" PARENT_SCOPE)
