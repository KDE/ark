set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

set(JSONINTERFACE_SOURCES
    jsonarchiveinterface.cpp
    jsonparser.cpp
)
add_library(jsoninterface STATIC ${JSONINTERFACE_SOURCES})
target_link_libraries(jsoninterface kerfuffle)

ecm_add_tests(
    archivetest.cpp
    createdialogtest.cpp
    mimetypetest.cpp
    LINK_LIBRARIES kerfuffle Qt5::Test
    NAME_PREFIX kerfuffle-)

ecm_add_tests(
    jobstest.cpp
    LINK_LIBRARIES jsoninterface Qt5::Test
    NAME_PREFIX kerfuffle-)
