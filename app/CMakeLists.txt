add_subdirectory(icons)

set(ark_SRCS
    batchextract.cpp
    main.cpp
    mainwindow.cpp
   )

ecm_qt_declare_logging_category(ark_SRCS
                                HEADER ark_debug.h
                                IDENTIFIER ARK
                                CATEGORY_NAME ark.main)

# For Mac and Windows.
file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/icons/*-apps-ark.png")

include(ECMAddAppIcon)
ecm_add_app_icon(ark_SRCS ICONS ${ICONS_SRCS})

add_executable( ark ${ark_SRCS} )

target_link_libraries(ark
    kerfuffle
    KF5::Crash
    KF5::DBusAddons
    KF5::KIOFileWidgets
    KF5::Parts)

# Remove duplicate mimetypes from list of supported formats.
list(REMOVE_DUPLICATES SUPPORTED_ARK_MIMETYPES)

configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/org.kde.ark.desktop.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/org.kde.ark.desktop
)

configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/ark_dndextract.desktop.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/ark_dndextract.desktop
)

install(TARGETS ark  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.ark.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/org.kde.ark.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES arkui.rc    DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/ark)

############ ark_extract_here plugin ###############

set(extracthere_SRCS batchextract.cpp extractHereDndPlugin.cpp ark_debug.cpp)

kcoreaddons_add_plugin(extracthere
    SOURCES ${extracthere_SRCS}
    INSTALL_NAMESPACE kf5/kio_dnd)

kcoreaddons_desktop_to_json(extracthere ${CMAKE_CURRENT_BINARY_DIR}/ark_dndextract.desktop DEFAULT_SERVICE_TYPE)

target_link_libraries(extracthere kerfuffle KF5::Parts KF5::KIOWidgets KF5::KIOFileWidgets)

# compressfileitemaction plugin

kcoreaddons_add_plugin(compressfileitemaction
    SOURCES compressfileitemaction.cpp
    JSON compressfileitemaction.json
    INSTALL_NAMESPACE "kf5/kfileitemaction")

target_link_libraries(compressfileitemaction
    kerfuffle
    KF5::I18n
    KF5::KIOWidgets)

# extractfileitemaction plugin

kcoreaddons_add_plugin(extractfileitemaction
    SOURCES extractfileitemaction.cpp
    JSON extractfileitemaction.json
    INSTALL_NAMESPACE "kf5/kfileitemaction")

target_link_libraries(extractfileitemaction
    kerfuffle
    KF5::I18n
    KF5::KIOWidgets)
