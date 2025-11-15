set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

include_directories(${CMAKE_SOURCE_DIR}/plugins/cliarjplugin/)

file(COPY ${CMAKE_BINARY_DIR}/plugins/cliarjplugin/kerfuffle_cliarj.json
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

ecm_add_test(
    cliarjtest.cpp
    ${CMAKE_SOURCE_DIR}/plugins/cliarjplugin/cliplugin.cpp
    ${CMAKE_BINARY_DIR}/plugins/cliarjplugin/ark_debug.cpp
    LINK_LIBRARIES testhelper kerfuffle Qt::Test
    TEST_NAME cliarjtest
)
