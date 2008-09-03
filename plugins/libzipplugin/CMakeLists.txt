
include_directories( ${LIBZIP_INCLUDE_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR}/../..
                     ${CMAKE_CURRENT_BINARY_DIR}/../..
                   )

if (LIBZIP_COMPILED_WITH_32BIT_OFF_T)
   remove_definitions(-D_FILE_OFFSET_BITS=64)
endif (LIBZIP_COMPILED_WITH_32BIT_OFF_T)

########### next target ###############

set(kerfuffle_libzip_SRCS zipplugin.cpp)

kde4_add_plugin(kerfuffle_libzip ${kerfuffle_libzip_SRCS})

target_link_libraries(kerfuffle_libzip  ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} ${LIBZIP_LIBRARY} kerfuffle )

install(TARGETS kerfuffle_libzip  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kerfuffle_libzip.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

