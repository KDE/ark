include_directories( ${LIBLZMA_INCLUDE_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR}/../..
                     ${CMAKE_CURRENT_BINARY_DIR}/../..
                   )

########### next target ###############

set(kerfuffle_liblzma_SRCS lzmaplugin.cpp)

kde4_add_plugin(kerfuffle_liblzma ${kerfuffle_liblzma_SRCS})

target_link_libraries(kerfuffle_liblzma  ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} ${LIBLZMA_LIBRARIES} kerfuffle )

install(TARGETS kerfuffle_liblzma  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES kerfuffle_liblzma.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

