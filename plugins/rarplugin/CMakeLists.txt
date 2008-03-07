
include_directories(
                     ${CMAKE_CURRENT_SOURCE_DIR}/../..
                     ${CMAKE_CURRENT_BINARY_DIR}/../..
                   )

########### next target ###############

set(kerfuffle_rar_SRCS rarplugin.cpp)

kde4_add_plugin(kerfuffle_rar ${kerfuffle_rar_SRCS})

target_link_libraries(kerfuffle_rar  ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} kerfuffle )

install(TARGETS kerfuffle_rar  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kerfuffle_rar.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

