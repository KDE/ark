include_directories(${LIBARCHIVE_INCLUDE_DIR})

########### next target ###############
set(SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES "application/x-deb;application/x-cd-image;application/x-bcpio;application/x-cpio;application/x-cpio-compressed;application/x-sv4cpio;application/x-sv4crc;")
set(SUPPORTED_LIBARCHIVE_READWRITE_MIMETYPES "application/x-tar;application/x-compressed-tar;application/x-bzip-compressed-tar;application/x-tarz;application/x-xz-compressed-tar;application/x-lzma-compressed-tar;")
if(HAVE_LIBARCHIVE_RPM_SUPPORT)
  set(SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES "${SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES}application/x-rpm;application/x-source-rpm;")
endif(HAVE_LIBARCHIVE_RPM_SUPPORT)
if(HAVE_LIBARCHIVE_CAB_SUPPORT)
  set(SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES "${SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES}application/vnd.ms-cab-compressed;")
endif(HAVE_LIBARCHIVE_CAB_SUPPORT)

# This MIME type was originally set in ark.desktop but is not mentioned anywhere else.
# Assuming that, if it were supported, it would be here.
set(SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES "${SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES}application/x-servicepack;")

configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_libarchive_readonly.desktop.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_libarchive_readonly.desktop
)

configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_libarchive.desktop.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_libarchive.desktop
)

set(kerfuffle_libarchive_SRCS libarchivehandler.cpp)

kde4_add_plugin(kerfuffle_libarchive ${kerfuffle_libarchive_SRCS})

target_link_libraries(kerfuffle_libarchive  ${KDE4_KIO_LIBS} ${KDE4_KDECORE_LIBS} ${LIBARCHIVE_LIBRARY} kerfuffle )

install(TARGETS kerfuffle_libarchive  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_libarchive.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_libarchive_readonly.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${SUPPORTED_LIBARCHIVE_READWRITE_MIMETYPES}${SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES}" PARENT_SCOPE)
