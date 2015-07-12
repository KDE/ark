set(kerfuffle_cli_SRCS cliplugin.cpp)
add_library(kerfuffle_cli ${kerfuffle_cli_SRCS})
target_link_libraries(kerfuffle_cli KF5::KIOCore kerfuffle)

install(TARGETS kerfuffle_cli DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kerfuffle_cli.desktop DESTINATION ${SERVICES_INSTALL_DIR})

