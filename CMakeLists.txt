project(ark)

add_subdirectory( part )
add_subdirectory( app )
add_subdirectory( pics )
add_subdirectory( plugins )
add_subdirectory( kerfuffle )
#add_subdirectory( old )



kde4_install_icons( ${ICON_INSTALL_DIR} )

