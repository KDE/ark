set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

add_executable(clirartest clirartest.cpp ../cliplugin.cpp)
add_test(ark-app-clirartest clirartest)
target_link_libraries(clirartest kerfuffle Qt5::Test KF5::Parts)
ecm_mark_as_test(clirartest)
