set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

include_directories(${CMAKE_SOURCE_DIR}/plugins/clirarplugin/)

add_executable(clirartest clirartest.cpp
    ${CMAKE_SOURCE_DIR}/plugins/clirarplugin/cliplugin.cpp
    ${CMAKE_BINARY_DIR}/plugins/clirarplugin/ark_debug.cpp)
add_test(ark-app-clirartest clirartest)
target_link_libraries(clirartest kerfuffle Qt5::Test KF5::Parts)
ecm_mark_as_test(clirartest)
