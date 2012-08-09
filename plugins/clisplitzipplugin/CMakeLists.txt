########### next target ###############

set(SUPPORTED_CLISPLITZIP_MIMETYPES "application/splitzip")

set(kerfuffle_clisplitzip_SRCS cliplugin.cpp)

kde4_add_plugin(kerfuffle_clisplitzip ${kerfuffle_clisplitzip_SRCS})

target_link_libraries(kerfuffle_clisplitzip  ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} kerfuffle )

configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_clisplitzip.desktop.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_clisplitzip.desktop
)

########### install files ###############

install(TARGETS kerfuffle_clisplitzip  DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_clisplitzip.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${SUPPORTED_CLISPLITZIP_MIMETYPES}" PARENT_SCOPE)
