set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

include_directories(${CMAKE_SOURCE_DIR}/plugins/cliunarchiverplugin/)

ecm_add_test(
    cliunarchivertest.cpp
    ${CMAKE_SOURCE_DIR}/plugins/cliunarchiverplugin/cliplugin.cpp
    ${CMAKE_BINARY_DIR}/plugins/cliunarchiverplugin/ark_debug.cpp
    LINK_LIBRARIES kerfuffle Qt5::Test
    TEST_NAME cliunarchivertest
    NAME_PREFIX plugins-)
