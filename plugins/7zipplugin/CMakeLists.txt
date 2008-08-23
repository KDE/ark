
include_directories(
                     ${CMAKE_CURRENT_SOURCE_DIR}/../..
                     ${CMAKE_CURRENT_BINARY_DIR}/../..
                   )

########### next target ###############

set(kerfuffle_7zip_SRCS 7zipplugin.cpp)

kde4_add_plugin(kerfuffle_7zip ${kerfuffle_7zip_SRCS})

target_link_libraries(kerfuffle_7zip  ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} kerfuffle )

install(TARGETS kerfuffle_7zip  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kerfuffle_7zip.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

