
include_directories( ${LIBARCHIVE_INCLUDE_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR}/../..
                     ${CMAKE_CURRENT_BINARY_DIR}/../..
                   )

########### next target ###############

set(kerfuffle_libarchive_SRCS libarchivehandler.cpp)

kde4_add_plugin(kerfuffle_libarchive ${kerfuffle_libarchive_SRCS})

target_link_libraries(kerfuffle_libarchive  ${KDE4_KDECORE_LIBS} ${LIBARCHIVE_LIBRARY} kerfuffle )

install(TARGETS kerfuffle_libarchive  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kerfuffle_libarchive.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

