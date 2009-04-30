
include_directories(
                     ${CMAKE_CURRENT_SOURCE_DIR}/../..
                     ${CMAKE_CURRENT_BINARY_DIR}/../..
                   )

########### next target ###############

set(kerfuffle_clizip_SRCS cliplugin.cpp)

kde4_add_plugin(kerfuffle_clizip ${kerfuffle_clizip_SRCS})

target_link_libraries(kerfuffle_clizip  ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} kerfuffle )



########### install files ###############

install(TARGETS kerfuffle_clizip  DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES kerfuffle_clizip.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

