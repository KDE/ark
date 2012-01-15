include_directories(${KERFUFFLE_QJSON_INCLUDE_DIR})

set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

set(JSONINTERFACE_SOURCES
    jsonarchiveinterface.cpp
    jsonparser.cpp
)
kde4_add_library(jsoninterface STATIC ${JSONINTERFACE_SOURCES})

macro(KERFUFFLE_UNIT_TESTS)
    foreach(_testname ${ARGN})
        kde4_add_unit_test(${_testname} NOGUI ${_testname}.cpp)
        target_link_libraries(${_testname} jsoninterface kerfuffle ${KDE4_KDEUI_LIBS} ${QT_QTTEST_LIBRARY} ${KERFUFFLE_QJSON_LIBRARIES})
    endforeach(_testname)
endmacro(KERFUFFLE_UNIT_TESTS)

KERFUFFLE_UNIT_TESTS(
    archivetest
    jobstest
)
