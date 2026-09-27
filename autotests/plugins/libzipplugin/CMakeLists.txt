set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

ecm_add_test(
    libziptest.cpp
    LINK_LIBRARIES testhelper kerfuffle Qt::Test
    TEST_NAME libziptest
)
