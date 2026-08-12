include_directories(${CMAKE_SOURCE_DIR}/app)

ecm_add_test(
    batchextracttest.cpp
    ${CMAKE_SOURCE_DIR}/app/batchextract.cpp
    ${CMAKE_BINARY_DIR}/app/ark_debug.cpp
    LINK_LIBRARIES Qt::Test KF6::KIOFileWidgets kerfuffle
    TEST_NAME batchextracttest
)

ecm_add_test(
    compressfileitemactiontest.cpp
    ${CMAKE_SOURCE_DIR}/app/compressfileitemaction.cpp
    LINK_LIBRARIES Qt::Test Qt::Widgets KF6::I18n KF6::KIOWidgets kerfuffle
    TEST_NAME compressfileitemactiontest
)
