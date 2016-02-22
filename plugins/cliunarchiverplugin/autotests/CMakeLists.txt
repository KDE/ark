set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

include_directories(${CMAKE_CURRENT_BINARY_DIR}/..)

add_executable(cliunarchivertest
    cliunarchivertest.cpp
    ../cliplugin.cpp
    ../ark_debug.cpp)

add_test(ark-app-cliunarchivertest cliunarchivertest)

target_link_libraries(cliunarchivertest
    kerfuffle
    Qt5::Test)

ecm_mark_as_test(cliunarchivertest)
