include_directories(
                     ${CMAKE_CURRENT_SOURCE_DIR}/..
                     ${CMAKE_CURRENT_BINARY_DIR}/..
                   )

ADD_DEFINITIONS(-D_LARGEFILE64_SOURCE)

set(arkpart_PART_SRCS
	part.cpp 
	infopanel.cpp
	arkviewer.cpp
	archivemodel.cpp
    )

#kde4_add_kcfg_files(arkpart_PART_SRCS settings.kcfgc)

#kde4_add_ui_files(arkpart_PART_SRCS general.ui )

kde4_add_plugin(arkpart WITH_PREFIX ${arkpart_PART_SRCS})


target_link_libraries(arkpart kerfuffle ${KDE4_KFILE_LIBS} ${KDE4_THREADWEAVER_LIBRARIES} ${LIBARCHIVE_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${KDE4_KPARTS_LIBS})

install(TARGETS arkpart  DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES ark_part.desktop     DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES ark_part.rc DESTINATION ${DATA_INSTALL_DIR}/ark )
