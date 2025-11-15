set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

include_directories(${CMAKE_SOURCE_DIR}/plugins/cli7zplugin/)

file(COPY ${CMAKE_BINARY_DIR}/plugins/cli7zplugin/kerfuffle_cli7z.json
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

ecm_add_test(
    cli7ztest.cpp
    ${CMAKE_SOURCE_DIR}/plugins/cli7zplugin/cliplugin.cpp
    ${CMAKE_BINARY_DIR}/plugins/cli7zplugin/ark_debug.cpp
    LINK_LIBRARIES testhelper kerfuffle Qt::Test
    TEST_NAME cli7ztest
)
