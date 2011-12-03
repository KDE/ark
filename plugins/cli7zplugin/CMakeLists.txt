########### next target ###############

set(SUPPORTED_CLI7Z_MIMETYPES "application/x-7z-compressed;")

set(kerfuffle_cli7z_SRCS cliplugin.cpp)

kde4_add_plugin(kerfuffle_cli7z ${kerfuffle_cli7z_SRCS})

target_link_libraries(kerfuffle_cli7z  ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} kerfuffle )

configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_cli7z.desktop.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_cli7z.desktop
)

########### install files ###############

install(TARGETS kerfuffle_cli7z  DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_cli7z.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${SUPPORTED_CLI7Z_MIMETYPES}" PARENT_SCOPE)
