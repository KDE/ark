include_directories(${CMAKE_SOURCE_DIR}/app)

ecm_add_test(
    batchextracttest.cpp
    ${CMAKE_SOURCE_DIR}/app/batchextract.cpp
    ${CMAKE_BINARY_DIR}/app/ark_debug.cpp
    LINK_LIBRARIES Qt::Test KF${KF_MAJOR_VERSION}::KIOFileWidgets kerfuffle
    TEST_NAME batchextracttest
    NAME_PREFIX app-)
