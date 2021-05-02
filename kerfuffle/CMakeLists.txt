########### next target ###############

set(kerfuffle_SRCS
    archiveformat.cpp
    archive_kerfuffle.cpp
    archiveinterface.cpp
    extractionsettingspage.cpp
    generalsettingspage.cpp
    previewsettingspage.cpp
    settingsdialog.cpp
    settingspage.cpp
    jobs.cpp
    adddialog.cpp
    compressionoptionswidget.cpp
    createdialog.cpp
    extractiondialog.cpp
    propertiesdialog.cpp
    queries.cpp
    addtoarchive.cpp
    cliinterface.cpp
    cliproperties.cpp
    mimetypes.cpp
    plugin.cpp
    pluginmanager.cpp
    pluginsettingspage.cpp
    archiveentry.cpp
    options.cpp
    qstringtokenizer.cpp
)

kconfig_add_kcfg_files(kerfuffle_SRCS settings.kcfgc GENERATE_MOC)

ki18n_wrap_ui(kerfuffle_SRCS
    createdialog.ui
    extractiondialog.ui
    extractionsettingspage.ui
    generalsettingspage.ui
    pluginsettingspage.ui
    previewsettingspage.ui
    propertiesdialog.ui
    compressionoptionswidget.ui
)

ecm_qt_declare_logging_category(kerfuffle_SRCS
                                HEADER ark_debug.h
                                IDENTIFIER ARK
                                CATEGORY_NAME ark.kerfuffle DESCRIPTION "Ark Kerfuffle" EXPORT ARK)

add_library(kerfuffle SHARED ${kerfuffle_SRCS})
generate_export_header(kerfuffle BASE_NAME kerfuffle)

if (APPLE)
   target_compile_definitions(kerfuffle PRIVATE -DDEPENDENCY_TOOL="otool")
   target_compile_definitions(kerfuffle PRIVATE -DDEPENDENCY_TOOL_ARGS="-L")
else()
   target_compile_definitions(kerfuffle PRIVATE -DDEPENDENCY_TOOL="ldd")
endif()

target_link_libraries(kerfuffle
PUBLIC
    KF5::Pty
    KF5::I18n
    KF5::WidgetsAddons
PRIVATE
    Qt5::Concurrent
    KF5::ConfigCore
    KF5::KIOCore
    KF5::KIOWidgets
    KF5::KIOFileWidgets
)

set_target_properties(kerfuffle PROPERTIES VERSION ${KERFUFFLE_VERSION} SOVERSION ${KERFUFFLE_SOVERSION})

install(TARGETS kerfuffle ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)

install(FILES kerfufflePlugin.desktop DESTINATION ${KDE_INSTALL_KSERVICETYPES5DIR})
install(FILES ark.kcfg                DESTINATION ${KDE_INSTALL_KCFGDIR})
