add_library(arkpart MODULE)

target_sources(arkpart PRIVATE
    factory.cpp
    part.cpp
    infopanel.cpp
    arkviewer.cpp
    archivemodel.cpp
    archivesortfiltermodel.cpp
    archiveview.cpp
    jobtracker.cpp
    overwritedialog.cpp
    arkpart.qrc
    )

ecm_qt_declare_logging_category(arkpart
                                HEADER ark_debug.h
                                IDENTIFIER ARK
                                CATEGORY_NAME ark.part DESCRIPTION "Ark KPart" EXPORT ARK)
set(ark_adaptor_SRCS)
qt_add_dbus_adaptor(ark_adaptor_SRCS dnddbusinterface.xml part.h Ark::Part)
target_sources(arkpart PRIVATE ${ark_adaptor_SRCS})
ki18n_wrap_ui(arkpart
    arkviewer.ui
    infopanel.ui
    jobtracker.ui)


target_link_libraries(arkpart kerfuffle KF${QT_MAJOR_VERSION}::Parts KF${QT_MAJOR_VERSION}::KIOFileWidgets KF${QT_MAJOR_VERSION}::ItemModels)

if(QT_MAJOR_VERSION STREQUAL "5")
    configure_file(
                ${CMAKE_CURRENT_SOURCE_DIR}/ark_part.desktop.cmake
                ${CMAKE_CURRENT_BINARY_DIR}/ark_part.desktop
    )

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ark_part.desktop DESTINATION ${KDE_INSTALL_KSERVICESDIR})
endif()

list(POP_BACK SUPPORTED_ARK_MIMETYPES)
list(JOIN SUPPORTED_ARK_MIMETYPES "\", \"" SUPPORTED_ARK_MIMETYPES_JSON)

configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/ark_part.json.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/ark_part.json
)

install(TARGETS arkpart DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf${QT_MAJOR_VERSION}/parts)
