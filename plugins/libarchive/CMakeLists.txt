include_directories(${LibArchive_INCLUDE_DIRS})

########### next target ###############
set(SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES "application/x-deb;application/x-cd-image;application/x-bcpio;application/x-cpio;application/x-cpio-compressed;application/x-sv4cpio;application/x-sv4crc;")
set(SUPPORTED_LIBARCHIVE_READWRITE_MIMETYPES "application/x-tar;application/x-compressed-tar;application/x-bzip-compressed-tar;application/x-tarz;application/x-xz-compressed-tar;application/x-lzma-compressed-tar;")
set(SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES "${SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES}application/x-rpm;application/x-source-rpm;")
set(SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES "${SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES}application/vnd.ms-cab-compressed;")

configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_libarchive_readonly.desktop.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_libarchive_readonly.desktop
)

configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_libarchive.desktop.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_libarchive.desktop
)

set(kerfuffle_libarchive_SRCS libarchivehandler.cpp)

add_library(kerfuffle_libarchive MODULE ${kerfuffle_libarchive_SRCS})

target_link_libraries(kerfuffle_libarchive KF5::KIOCore ${LibArchive_LIBRARIES} kerfuffle )

install(TARGETS kerfuffle_libarchive  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_libarchive.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_libarchive_readonly.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${SUPPORTED_LIBARCHIVE_READWRITE_MIMETYPES}${SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES}" PARENT_SCOPE)
