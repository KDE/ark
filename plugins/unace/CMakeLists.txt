
include_directories(
                     ${CMAKE_CURRENT_SOURCE_DIR}/../..
                     ${CMAKE_CURRENT_BINARY_DIR}/../..
                   )

########### next target ###############

set(kerfuffle_unace_SRCS unaceplugin.cpp)

kde4_add_plugin(kerfuffle_unace ${kerfuffle_unace_SRCS})

target_link_libraries(kerfuffle_unace  ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} kerfuffle )

install(TARGETS kerfuffle_unace  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kerfuffle_unace.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

