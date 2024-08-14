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

    factory.h
    part.h
    infopanel.h
    arkviewer.h
    archivemodel.h
    archivesortfiltermodel.h
    archiveview.h
    jobtracker.h
    overwritedialog.h

    arkpart.qrc
    )

ecm_qt_declare_logging_category(arkpart
                                HEADER ark_debug.h
                                IDENTIFIER ARK_LOG
                                CATEGORY_NAME ark.part DESCRIPTION "Ark KPart" EXPORT ARK)
set(ark_adaptor_SRCS)
qt_add_dbus_adaptor(ark_adaptor_SRCS dnddbusinterface.xml part.h Ark::Part)
target_sources(arkpart PRIVATE ${ark_adaptor_SRCS})
ki18n_wrap_ui(arkpart
    arkviewer.ui
    infopanel.ui
    jobtracker.ui)


target_link_libraries(arkpart kerfuffle KF6::Parts KF6::KIOFileWidgets Qt6::DBus)

list(POP_BACK SUPPORTED_ARK_MIMETYPES)
list(JOIN SUPPORTED_ARK_MIMETYPES "\", \"" SUPPORTED_ARK_MIMETYPES_JSON)

configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/ark_part.json.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/ark_part.json
)

install(TARGETS arkpart DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/parts)
