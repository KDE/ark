set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

include_directories(${CMAKE_SOURCE_DIR}/plugins/cliunarchiverplugin/)

add_executable(cliunarchivertest
    cliunarchivertest.cpp
    ${CMAKE_SOURCE_DIR}/plugins/cliunarchiverplugin/cliplugin.cpp
    ${CMAKE_BINARY_DIR}/plugins/cliunarchiverplugin/ark_debug.cpp)

add_test(ark-app-cliunarchivertest cliunarchivertest)

target_link_libraries(cliunarchivertest
    kerfuffle
    Qt5::Test)

ecm_mark_as_test(cliunarchivertest)
