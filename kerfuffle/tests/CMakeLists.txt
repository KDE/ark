set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

macro(KERFUFFLE_UNIT_TESTS)
    foreach(_testname ${ARGN})
        kde4_add_unit_test(${_testname} NOGUI ${_testname}.cpp)
        target_link_libraries(${_testname} kerfuffle ${KDE4_KDEUI_LIBS} ${QT_QTTEST_LIBRARY})
    endforeach(_testname)
endmacro(KERFUFFLE_UNIT_TESTS)

KERFUFFLE_UNIT_TESTS(
    archivetest
    jobstest
)
