set(arkpart_PART_SRCS
	part.cpp
	infopanel.cpp
	arkviewer.cpp
	archivemodel.cpp
	archiveview.cpp
	jobtracker.cpp
        archiveconflictdialog.cpp
    )

qt4_add_dbus_adaptor(arkpart_PART_SRCS dnddbusinterface.xml part.h Ark::Part)

kde4_add_ui_files(arkpart_PART_SRCS infopanel.ui )
kde4_add_ui_files(arkpart_PART_SRCS jobtracker.ui )

kde4_add_plugin(arkpart ${arkpart_PART_SRCS})

target_link_libraries(arkpart kerfuffle konq ${KDE4_KFILE_LIBS} ${KDE4_KPARTS_LIBS} ${KDE4_KHTML_LIBS} ${KDE4_KDEUI_LIBS})

configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/ark_part.desktop.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/ark_part.desktop
)

install(TARGETS arkpart  DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/ark_part.desktop     DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES ark_part.rc          DESTINATION ${DATA_INSTALL_DIR}/ark )
