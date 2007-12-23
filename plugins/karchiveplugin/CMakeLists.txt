
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../..
                     ${CMAKE_CURRENT_BINARY_DIR}/../..
                   )

########### next target ###############

set(kerfuffle_karchive_SRCS karchiveplugin.cpp)

kde4_add_plugin(kerfuffle_karchive ${kerfuffle_karchive_SRCS})

target_link_libraries(kerfuffle_karchive  ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} kerfuffle )

install(TARGETS kerfuffle_karchive  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kerfuffle_karchive.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

