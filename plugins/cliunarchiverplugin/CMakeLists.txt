include_directories(
                     ${CMAKE_CURRENT_SOURCE_DIR}/../..
                     ${CMAKE_CURRENT_BINARY_DIR}/../..
                   )

########### next target ###############

set(kerfuffle_cliunarchiver_SRCS cliplugin.cpp)

kde4_add_plugin(kerfuffle_cliunarchiver ${kerfuffle_cliunarchiver_SRCS})

target_link_libraries(kerfuffle_cliunarchiver  ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} kerfuffle )



########### install files ###############

install(TARGETS kerfuffle_cliunarchiver  DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES kerfuffle_cliunarchiver.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )