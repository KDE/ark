project(ark)

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_subdirectory(part)
add_subdirectory(app)
add_subdirectory(plugins)
add_subdirectory(kerfuffle)
