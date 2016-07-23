set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

include_directories(${CMAKE_SOURCE_DIR}/plugins/libarchive/)

file(COPY ${CMAKE_BINARY_DIR}/plugins/libarchive/kerfuffle_libarchive.json
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

ecm_add_test(
    libarchivetest.cpp
    ${CMAKE_SOURCE_DIR}/plugins/libarchive/libarchiveplugin.cpp
    ${CMAKE_SOURCE_DIR}/plugins/libarchive/readwritelibarchiveplugin.cpp
    ${CMAKE_BINARY_DIR}/plugins/libarchive/ark_debug.cpp
    LINK_LIBRARIES testhelper ${LibArchive_LIBRARIES} kerfuffle Qt5::Test
    TEST_NAME libarchivetest
    NAME_PREFIX plugins-)
