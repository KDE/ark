include_directories(
                     ${CMAKE_CURRENT_SOURCE_DIR}/..
                     ${CMAKE_CURRENT_BINARY_DIR}/..
                   )

ADD_DEFINITIONS(-D_LARGEFILE64_SOURCE)

set(libark_common_SRCS archiveformatinfo.cpp )
kde4_add_kcfg_files(libark_common_SRCS settings.kcfgc)

########### next target ###############

set(ark_KDEINIT_SRCS main.cpp arkapp.cpp mainwindow.cpp ${libark_common_SRCS})

kde4_add_ui_files(ark_KDEINIT_SRCS general.ui )

kde4_add_kdeinit_executable( oldark ${ark_KDEINIT_SRCS})

target_link_libraries(kdeinit_oldark ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${KDE4_KPARTS_LIBS} kerfuffle)

install(TARGETS kdeinit_oldark  DESTINATION ${LIB_INSTALL_DIR})

target_link_libraries( oldark kdeinit_oldark )
install(TARGETS oldark  DESTINATION ${BIN_INSTALL_DIR} )

########### next target ###############

set(arkpart_PART_SRCS ${libark_common_SRCS} 
    ark_part.cpp 
    filelistview.cpp 
    arkutils.cpp 
    arkwidget.cpp 
    arkviewer.cpp 
    extractiondialog.cpp
   )

kde4_add_ui_files(arkpart_PART_SRCS general.ui )

kde4_add_plugin(oldarkpart WITH_PREFIX ${arkpart_PART_SRCS})


target_link_libraries(oldarkpart kerfuffle ${KDE4_KFILE_LIBS} ${KDE4_THREADWEAVER_LIBRARIES} ${LIBARCHIVE_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${KDE4_KPARTS_LIBS})

install(TARGETS oldarkpart  DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

#install( FILES ark.desktop          DESTINATION ${XDG_APPS_DIR} )
#install( FILES ark.kcfg             DESTINATION ${KCFG_INSTALL_DIR} )
install( FILES ark_part.desktop     DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES arkui.rc ark_part.rc DESTINATION ${DATA_INSTALL_DIR}/ark )
