set(arkpart_PART_SRCS
    factory.cpp
    part.cpp
    infopanel.cpp
    arkviewer.cpp
    archivemodel.cpp
    archivesortfiltermodel.cpp
    archiveview.cpp
    jobtracker.cpp
    overwritedialog.cpp
    )

qt5_add_resources(arkpart_PART_SRCS arkpart.qrc)

ecm_qt_declare_logging_category(arkpart_PART_SRCS
                                HEADER ark_debug.h
                                IDENTIFIER ARK
                                CATEGORY_NAME ark.part)

qt5_add_dbus_adaptor(arkpart_PART_SRCS dnddbusinterface.xml part.h Ark::Part)

ki18n_wrap_ui(arkpart_PART_SRCS
    arkviewer.ui
    infopanel.ui
    jobtracker.ui)

add_library(arkpart MODULE ${arkpart_PART_SRCS})

target_link_libraries(arkpart kerfuffle KF5::Parts KF5::KIOFileWidgets KF5::ItemModels)

configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/ark_part.desktop.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/ark_part.desktop
)

kcoreaddons_desktop_to_json(arkpart ${CMAKE_CURRENT_BINARY_DIR}/ark_part.desktop SERVICE_TYPES kpart.desktop browserview.desktop)

install(TARGETS arkpart DESTINATION ${KDE_INSTALL_PLUGINDIR})

########### install files ###############

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ark_part.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install(FILES ark_viewer.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/ark)
