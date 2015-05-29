add_subdirectory(icons)

set(ark_SRCS
    batchextract.cpp
    main.cpp
    mainwindow.cpp
   )

# For Mac and Windows.
file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/icons/*-apps-ark.png")

include(ECMAddAppIcon)
ecm_add_app_icon(ark_SRCS ICONS ${ICONS_SRCS})

add_executable( ark ${ark_SRCS} )

target_link_libraries( ark kerfuffle KF5::DBusAddons KF5::Parts )

configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/org.kde.ark.desktop.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/org.kde.ark.desktop
)

configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/ark_dndextract.desktop.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/ark_dndextract.desktop
)

configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/ark_servicemenu.desktop.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/ark_servicemenu.desktop
)

install( TARGETS ark  ${INSTALL_TARGETS_DEFAULT_ARGS} )
install(FILES ark_addtoservicemenu.desktop ${CMAKE_CURRENT_BINARY_DIR}/ark_servicemenu.desktop DESTINATION ${SERVICES_INSTALL_DIR}/ServiceMenus)
install( PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.ark.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
install( FILES ${CMAKE_CURRENT_SOURCE_DIR}/ark.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )
install( FILES arkui.rc    DESTINATION ${KXMLGUI_INSTALL_DIR}/ark )

############ ark_extract_here plugin ###############

set(extracthere_SRCS batchextract.cpp extractHereDndPlugin.cpp)

add_library(extracthere MODULE ${extracthere_SRCS})
kcoreaddons_desktop_to_json(extracthere ${CMAKE_CURRENT_BINARY_DIR}/ark_dndextract.desktop)

target_link_libraries(extracthere kerfuffle KF5::Parts KF5::KIOWidgets)
install( TARGETS extracthere DESTINATION ${PLUGIN_INSTALL_DIR}/kf5/kio_dnd )
