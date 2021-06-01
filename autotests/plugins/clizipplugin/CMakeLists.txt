set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

include_directories(${CMAKE_SOURCE_DIR}/plugins/clizipplugin/)

file(COPY ${CMAKE_BINARY_DIR}/plugins/clizipplugin/kerfuffle_clizip.json
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

ecm_add_test(
    cliziptest.cpp
    ${CMAKE_SOURCE_DIR}/plugins/clizipplugin/cliplugin.cpp
    ${CMAKE_BINARY_DIR}/plugins/clizipplugin/ark_debug.cpp
    LINK_LIBRARIES testhelper kerfuffle Qt::Test
    TEST_NAME cliziptest
    NAME_PREFIX plugins-)
