project(ark)

add_subdirectory( part )
add_subdirectory( app )
add_subdirectory( plugins )
if( NOT WIN32 )
  add_subdirectory( kerfuffle )
endif( NOT WIN32 )
