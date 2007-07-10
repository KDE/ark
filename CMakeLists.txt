project(ark)

add_subdirectory( pics ) 

ADD_DEFINITIONS(-D_LARGEFILE64_SOURCE)

set(libark_common_SRCS archiveformatinfo.cpp )
kde4_add_kcfg_files(libark_common_SRCS settings.kcfgc)

########### next target ###############

set(ark_KDEINIT_SRCS main.cpp arkapp.cpp mainwindow.cpp ${libark_common_SRCS})

#kde4_automoc(${ark_KDEINIT_SRCS})

kde4_add_ui_files(ark_KDEINIT_SRCS general.ui )

kde4_add_kdeinit_executable( ark ${ark_KDEINIT_SRCS})

target_link_libraries(kdeinit_ark ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${KDE4_KPARTS_LIBS})

install(TARGETS kdeinit_ark  DESTINATION ${LIB_INSTALL_DIR})

target_link_libraries( ark kdeinit_ark )
install(TARGETS ark  DESTINATION ${BIN_INSTALL_DIR} )

########### next target ###############

set(arkpart_PART_SRCS ${libark_common_SRCS} 
   ark_part.cpp 
   arkfactory.cpp 
   filelistview.cpp 
   arkutils.cpp 
   archiveformatdlg.cpp 
   arkwidget.cpp 
   arkviewer.cpp 
   extractiondialog.cpp
   arch.cpp 
   archiveinterface.cpp
   libarchivehandler.cpp
   jobs.cpp
   archivebase.cpp
   #tar.cpp 
   #zip.cpp 
   #lha.cpp 
   #compressedfile.cpp 
   #zoo.cpp 
   #rar.cpp 
   #ar.cpp 
   #sevenzip.cpp
   )

#kde4_automoc(${arkpart_PART_SRCS})

kde4_add_ui_files(arkpart_PART_SRCS general.ui )

kde4_add_plugin(arkpart WITH_PREFIX ${arkpart_PART_SRCS})


target_link_libraries(arkpart ${KDE4_KFILE_LIBS} ${KDE4_THREADWEAVER_LIBRARIES} ${LIBARCHIVE_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${KDE4_KPARTS_LIBS})

install(TARGETS arkpart  DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES ark.desktop  DESTINATION ${XDG_APPS_DIR})
install( FILES ark.kcfg  DESTINATION ${KCFG_INSTALL_DIR})
install( FILES ark_part.desktop  DESTINATION ${SERVICES_INSTALL_DIR})
install( FILES arkui.rc ark_part.rc ark_part_readonly.rc  DESTINATION ${DATA_INSTALL_DIR}/ark)

kde4_install_icons( ${ICON_INSTALL_DIR}   )




