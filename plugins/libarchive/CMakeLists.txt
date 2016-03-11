include_directories(${LibArchive_INCLUDE_DIRS})

########### next target ###############
set(SUPPORTED_LIBARCHIVE_READWRITE_MIMETYPES "application/x-tar;application/x-compressed-tar;application/x-bzip-compressed-tar;application/x-tarz;application/x-xz-compressed-tar;")
set(SUPPORTED_LIBARCHIVE_READWRITE_MIMETYPES "${SUPPORTED_LIBARCHIVE_READWRITE_MIMETYPES}application/x-lzma-compressed-tar;application/x-lzip;application/x-tzo;")
set(SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES "application/x-deb;application/x-cd-image;application/x-bcpio;application/x-cpio;application/x-cpio-compressed;application/x-sv4cpio;application/x-sv4crc;")
set(SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES "${SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES}application/x-rpm;application/x-source-rpm;application/vnd.ms-cab-compressed;")

configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_libarchive_readonly.desktop.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_libarchive_readonly.desktop
)

configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_libarchive.desktop.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_libarchive.desktop
)

set(kerfuffle_libarchive_readonly_SRCS libarchiveplugin.cpp readonlylibarchiveplugin.cpp ark_debug.cpp)
set(kerfuffle_libarchive_readwrite_SRCS libarchiveplugin.cpp readwritelibarchiveplugin.cpp ark_debug.cpp)
set(kerfuffle_libarchive_SRCS ${kerfuffle_libarchive_readonly_SRCS} readwritelibarchiveplugin.cpp)

ecm_qt_declare_logging_category(kerfuffle_libarchive_SRCS
                                HEADER ark_debug.h
                                IDENTIFIER ARK
                                CATEGORY_NAME ark.libarchive)

add_library(kerfuffle_libarchive_readonly MODULE ${kerfuffle_libarchive_readonly_SRCS})
add_library(kerfuffle_libarchive MODULE ${kerfuffle_libarchive_readwrite_SRCS})

target_link_libraries(kerfuffle_libarchive_readonly KF5::KIOCore ${LibArchive_LIBRARIES} kerfuffle)
target_link_libraries(kerfuffle_libarchive KF5::KIOCore ${LibArchive_LIBRARIES} kerfuffle)

kcoreaddons_desktop_to_json(kerfuffle_libarchive_readonly ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_libarchive_readonly.desktop)
kcoreaddons_desktop_to_json(kerfuffle_libarchive ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_libarchive.desktop)

install(TARGETS kerfuffle_libarchive_readonly DESTINATION ${PLUGIN_INSTALL_DIR}/kerfuffle)
install(TARGETS kerfuffle_libarchive DESTINATION ${PLUGIN_INSTALL_DIR}/kerfuffle)

set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${SUPPORTED_LIBARCHIVE_READWRITE_MIMETYPES}${SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES}" PARENT_SCOPE)
