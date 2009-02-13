
include_directories(
                     ${CMAKE_CURRENT_SOURCE_DIR}/../..
                     ${CMAKE_CURRENT_BINARY_DIR}/../..
                   )

########### next target ###############

set(kerfuffle_clirar_SRCS cliplugin.cpp)

kde4_add_plugin(kerfuffle_clirar ${kerfuffle_clirar_SRCS})

target_link_libraries(kerfuffle_clirar  ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} kerfuffle )

########### install files ###############

install(TARGETS kerfuffle_clirar  DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES kerfuffle_clirar.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

