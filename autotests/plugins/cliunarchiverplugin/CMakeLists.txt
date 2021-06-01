set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

include_directories(${CMAKE_SOURCE_DIR}/plugins/cliunarchiverplugin/)

file(COPY ${CMAKE_BINARY_DIR}/plugins/cliunarchiverplugin/kerfuffle_cliunarchiver.json
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

ecm_add_test(
    cliunarchivertest.cpp
    ${CMAKE_SOURCE_DIR}/plugins/cliunarchiverplugin/cliplugin.cpp
    ${CMAKE_BINARY_DIR}/plugins/cliunarchiverplugin/ark_debug.cpp
    LINK_LIBRARIES testhelper kerfuffle Qt::Test
    TEST_NAME cliunarchivertest
    NAME_PREFIX plugins-)

# cliunarchiver plugin needs exceptions enabled
kde_target_enable_exceptions(cliunarchivertest PRIVATE)
