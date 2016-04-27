include_directories(${LibArchive_INCLUDE_DIRS})

########### next target ###############
set(SUPPORTED_LIBARCHIVE_READWRITE_MIMETYPES "application/x-tar;application/x-compressed-tar;application/x-bzip-compressed-tar;application/x-tarz;application/x-xz-compressed-tar;")
set(SUPPORTED_LIBARCHIVE_READWRITE_MIMETYPES "${SUPPORTED_LIBARCHIVE_READWRITE_MIMETYPES}application/x-lzma-compressed-tar;application/x-lzip-compressed-tar;application/x-tzo;application/x-lrzip-compressed-tar;")
set(SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES "application/vnd.debian.binary-package;application/x-deb;application/x-cd-image;application/x-bcpio;application/x-cpio;application/x-cpio-compressed;application/x-sv4cpio;application/x-sv4crc;")
set(SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES "${SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES}application/x-rpm;application/x-source-rpm;application/vnd.ms-cab-compressed;")

set(INSTALLED_LIBARCHIVE_PLUGINS "")

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

install(TARGETS kerfuffle_libarchive_readonly DESTINATION ${KDE_INSTALL_PLUGINDIR}/kerfuffle)
set(INSTALLED_LIBARCHIVE_PLUGINS "${INSTALLED_LIBARCHIVE_PLUGINS}kerfuffle_libarchive_readonly;")

install(TARGETS kerfuffle_libarchive DESTINATION ${KDE_INSTALL_PLUGINDIR}/kerfuffle)
set(INSTALLED_LIBARCHIVE_PLUGINS "${INSTALLED_LIBARCHIVE_PLUGINS}kerfuffle_libarchive;")

set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${SUPPORTED_LIBARCHIVE_READWRITE_MIMETYPES}${SUPPORTED_LIBARCHIVE_READONLY_MIMETYPES}" PARENT_SCOPE)
set(INSTALLED_KERFUFFLE_PLUGINS "${INSTALLED_KERFUFFLE_PLUGINS}${INSTALLED_LIBARCHIVE_PLUGINS}" PARENT_SCOPE)
