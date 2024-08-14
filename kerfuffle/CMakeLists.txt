########### next target ###############

add_library(kerfuffle SHARED)
target_sources(kerfuffle PRIVATE
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
    metadatabackup.cpp


    archiveformat.h
    archive_kerfuffle.h
    archiveinterface.h
    extractionsettingspage.h
    generalsettingspage.h
    previewsettingspage.h
    settingsdialog.h
    settingspage.h
    jobs.h
    adddialog.h
    compressionoptionswidget.h
    createdialog.h
    extractiondialog.h
    propertiesdialog.h
    queries.h
    addtoarchive.h
    cliinterface.h
    cliproperties.h
    mimetypes.h
    plugin.h
    pluginmanager.h
    pluginsettingspage.h
    archiveentry.h
    options.h
    qstringtokenizer.h
    metadatabackup.h
)

kconfig_add_kcfg_files(kerfuffle settings.kcfgc GENERATE_MOC)

ki18n_wrap_ui(kerfuffle
    createdialog.ui
    extractiondialog.ui
    extractionsettingspage.ui
    generalsettingspage.ui
    pluginsettingspage.ui
    previewsettingspage.ui
    propertiesdialog.ui
    compressionoptionswidget.ui
)

ecm_qt_declare_logging_category(kerfuffle
                                HEADER ark_debug.h
                                IDENTIFIER ARK_LOG
                                CATEGORY_NAME ark.kerfuffle DESCRIPTION "Ark Kerfuffle" EXPORT ARK)

generate_export_header(kerfuffle BASE_NAME kerfuffle)

if (APPLE)
   target_compile_definitions(kerfuffle PRIVATE -DDEPENDENCY_TOOL="otool")
   target_compile_definitions(kerfuffle PRIVATE -DDEPENDENCY_TOOL_ARGS="-L")
else()
   target_compile_definitions(kerfuffle PRIVATE -DDEPENDENCY_TOOL="ldd")
endif()

target_link_libraries(kerfuffle
PUBLIC
    KF6::FileMetaData
    KF6::I18n
    KF6::WidgetsAddons
    KF6::CoreAddons
PRIVATE
    Qt6::Concurrent
    KF6::ConfigCore
    KF6::ConfigWidgets
    KF6::KIOCore
    KF6::KIOWidgets
    KF6::KIOFileWidgets
)

if (NOT WIN32)
    target_link_libraries(kerfuffle
    PUBLIC
        KF6::Pty
    )
endif()

set_target_properties(kerfuffle PROPERTIES VERSION ${KERFUFFLE_VERSION} SOVERSION ${KERFUFFLE_SOVERSION})

install(TARGETS kerfuffle ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)

install(FILES ark.kcfg                DESTINATION ${KDE_INSTALL_KCFGDIR})
