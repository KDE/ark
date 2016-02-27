set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

set(JSONINTERFACE_SOURCES
    jsonarchiveinterface.cpp
    jsonparser.cpp
)
add_library(jsoninterface STATIC ${JSONINTERFACE_SOURCES})
target_link_libraries(jsoninterface kerfuffle)

macro(KERFUFFLE_UNIT_TESTS)
    foreach(_testname ${ARGN})
        add_executable(${_testname} ${_testname}.cpp)
        add_test(kerfuffle-${_testname} ${_testname})
        target_link_libraries(${_testname} jsoninterface kerfuffle Qt5::Test)
        ecm_mark_as_test(${_testname})
        ecm_mark_nongui_executable(${_testname})
    endforeach(_testname)
endmacro(KERFUFFLE_UNIT_TESTS)

KERFUFFLE_UNIT_TESTS(
    archivetest
    jobstest
    mimetypetest
    createdialogtest
)
