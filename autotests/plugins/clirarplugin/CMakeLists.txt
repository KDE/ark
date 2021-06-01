set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

include_directories(${CMAKE_SOURCE_DIR}/plugins/clirarplugin/)

file(COPY ${CMAKE_BINARY_DIR}/plugins/clirarplugin/kerfuffle_clirar.json
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

ecm_add_test(
    clirartest.cpp
    ${CMAKE_SOURCE_DIR}/plugins/clirarplugin/cliplugin.cpp
    ${CMAKE_BINARY_DIR}/plugins/clirarplugin/ark_debug.cpp
    LINK_LIBRARIES testhelper kerfuffle Qt::Test
    TEST_NAME clirartest
    NAME_PREFIX plugins-)
