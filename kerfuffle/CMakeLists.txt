########### next target ###############

set(kerfuffle_SRCS
    archive_kerfuffle.cpp
    archiveinterface.cpp
    extractionsettingspage.cpp
    previewsettingspage.cpp
    settingspage.cpp
    jobs.cpp
    createdialog.cpp
    extractiondialog.cpp
    adddialog.cpp
    queries.cpp
    addtoarchive.cpp
    cliinterface.cpp
   )

kconfig_add_kcfg_files(kerfuffle_SRCS settings.kcfgc)

ki18n_wrap_ui(kerfuffle_SRCS
    adddialog.ui
    createdialog.ui
    extractiondialog.ui
    extractionsettings.ui
    previewsettings.ui
)

ecm_qt_declare_logging_category(kerfuffle_SRCS
                                HEADER ark_debug.h
                                IDENTIFIER ARK
                                CATEGORY_NAME ark.kerfuffle)

add_library(kerfuffle SHARED ${kerfuffle_SRCS})
generate_export_header(kerfuffle BASE_NAME kerfuffle)

target_link_libraries(kerfuffle
PUBLIC
    KF5::IconThemes
    KF5::Pty
    KF5::Service
    KF5::I18n
PRIVATE
    KF5::KIOCore
    KF5::KIOWidgets
    KF5::KIOFileWidgets
    KF5::WidgetsAddons

)
if (NOT WIN32)
  target_link_libraries(kerfuffle ${KDE4_KPTY_LIBS})
endif (NOT WIN32)

set_target_properties(kerfuffle PROPERTIES VERSION ${KERFUFFLE_VERSION_STRING} SOVERSION ${KERFUFFLE_SOVERSION})

install(TARGETS kerfuffle ${INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)

install(FILES kerfufflePlugin.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})
install(FILES ark.kcfg                DESTINATION ${KCFG_INSTALL_DIR})
