set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

set(TESTHELPER_SOURCES
    testhelper.h
    testhelper.cpp
)

add_library(testhelper STATIC ${TESTHELPER_SOURCES})
target_link_libraries(testhelper Qt5::Test kerfuffle)
