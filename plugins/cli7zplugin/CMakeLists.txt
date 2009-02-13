
include_directories(
                     ${CMAKE_CURRENT_SOURCE_DIR}/../..
                     ${CMAKE_CURRENT_BINARY_DIR}/../..
                   )

########### next target ###############

set(kerfuffle_7z_SRCS cliplugin.cpp)

kde4_add_plugin(kerfuffle_7z ${kerfuffle_7z_SRCS})

target_link_libraries(kerfuffle_7z  ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} kerfuffle )



########### install files ###############

install(TARGETS kerfuffle_7z  DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES kerfuffle_7z.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

