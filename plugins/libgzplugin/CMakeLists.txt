
include_directories( ${ZLIB_INCLUDE_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR}/../..
                     ${CMAKE_CURRENT_BINARY_DIR}/../..
                   )

########### next target ###############

set(kerfuffle_libgz_SRCS gzplugin.cpp)

kde4_add_plugin(kerfuffle_libgz ${kerfuffle_libgz_SRCS})

target_link_libraries(kerfuffle_libgz  ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} ${ZLIB_LIBRARIES} kerfuffle )

install(TARGETS kerfuffle_libgz  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kerfuffle_libgz.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

