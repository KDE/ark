project(ark)

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=1601)

add_subdirectory(part)
add_subdirectory(app)
add_subdirectory(plugins)
add_subdirectory(kerfuffle)
