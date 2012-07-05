
########### next target ###############

set(SUPPORTED_CLI7ZZIP_MIMETYPES "application/x-java-archive;application/zip;")

set(kerfuffle_cli7zzip_SRCS cliplugin.cpp)

kde4_add_plugin(kerfuffle_cli7zzip ${kerfuffle_cli7zzip_SRCS})

target_link_libraries(kerfuffle_cli7zzip  ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} kerfuffle )

configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_cli7zzip.desktop.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_cli7zzip.desktop
)

########### install files ###############

install(TARGETS kerfuffle_cli7zzip  DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_cli7zzip.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${SUPPORTED_CLI7ZZIP_MIMETYPES}" PARENT_SCOPE)
