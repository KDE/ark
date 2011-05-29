project(ark)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    find_package(KDE4)
    include(KDE4Defaults)
    include_directories(${KDE4_INCLUDES})
endif()

include( MacroLibrary )
list(APPEND CMAKE_MODULE_PATH ${ark_SOURCE_DIR}/cmake)

macro_optional_find_package(LibArchive)
macro_log_feature(LIBARCHIVE_FOUND "LibArchive" "A library for dealing with a wide variety of archive file formats" "http://code.google.com/p/libarchive/" FALSE "" "Required for among others tar, tar.gz, tar.bz2 formats in Ark.")

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_subdirectory(part)
add_subdirectory(app)
add_subdirectory(plugins)
add_subdirectory(kerfuffle)
add_subdirectory(doc)
