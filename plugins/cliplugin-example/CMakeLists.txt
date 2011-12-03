set(kerfuffle_cli_SRCS cliplugin.cpp)
kde4_add_plugin(kerfuffle_cli ${kerfuffle_cli_SRCS})
target_link_libraries(kerfuffle_cli ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} kerfuffle)

install(TARGETS kerfuffle_cli DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kerfuffle_cli.desktop DESTINATION ${SERVICES_INSTALL_DIR})

