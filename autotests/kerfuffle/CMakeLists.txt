set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

set(JSONINTERFACE_SOURCES
    jsonarchiveinterface.cpp
    jsonparser.cpp
)
add_library(jsoninterface STATIC ${JSONINTERFACE_SOURCES})
target_link_libraries(jsoninterface kerfuffle)

ecm_add_tests(
    addtoarchivetest.cpp
    deletetest.cpp
    loadtest.cpp
    extracttest.cpp
    addtest.cpp
    movetest.cpp
    copytest.cpp
    createdialogtest.cpp
    metadatatest.cpp
    mimetypetest.cpp
    preservemetadatatest.cpp
    LINK_LIBRARIES testhelper kerfuffle Qt::Test KF6::ConfigCore KF6::KIOCore
)

ecm_add_test(
    adddialogtest.cpp
    LINK_LIBRARIES kerfuffle Qt::Test KF6::KIOFileWidgets
)

ecm_add_tests(
    jobstest.cpp
    LINK_LIBRARIES jsoninterface Qt::Test
)

ecm_add_tests(
    queryjobtest.cpp
    LINK_LIBRARIES kerfuffle Qt::Test
)

# A job that waits for its thread while that thread waits for an answer never gets there,
# so this one ends by running out of time rather than by failing an assertion.
set_tests_properties(kerfuffle-queryjobtest PROPERTIES TIMEOUT 30)

# metadatatest needs the number of plugins actually installed by an install() command.
list(REMOVE_ITEM INSTALLED_KERFUFFLE_PLUGINS "")
list(LENGTH INSTALLED_KERFUFFLE_PLUGINS INSTALLED_COUNT)
target_compile_definitions(metadatatest PRIVATE -DPLUGINS_COUNT=${INSTALLED_COUNT})
