include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )
set( ark_SRCS main.cpp
              mainwindow.cpp
   )

kde4_add_app_icon(ark_SRCS "${KDE4_INSTALL_DIR}/share/icons/oxygen/*/apps/utilities-file-archiver.png")

kde4_add_executable( ark ${ark_SRCS} )

target_link_libraries( ark kerfuffle ${KDE4_KFILE_LIBS} ${KDE4_KPARTS_LIBS} )

install( TARGETS ark  ${INSTALL_TARGETS_DEFAULT_ARGS} )
install(FILES ark_addtoservicemenu.desktop ark_servicemenu.desktop DESTINATION ${SERVICES_INSTALL_DIR}/ServiceMenus)
install( FILES ark.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
install( FILES arkui.rc    DESTINATION ${DATA_INSTALL_DIR}/ark )

########### konqueror ark_extract_here plugin ###############
find_package(LibKonq)
if (LIBKONQ_FOUND)
   set(extracthere_SRCS extractHereDndPlugin.cpp)
   kde4_add_plugin(extracthere WITH_PREFIX ${extracthere_SRCS})
   target_link_libraries(extracthere kerfuffle ${KDE4_KDECORE_LIBS} ${KDE4_KPARTS_LIBS} ${LIBKONQ_LIBRARY} )
   include_directories(${LIBKONQ_INCLUDE_DIR})
   install( TARGETS extracthere DESTINATION ${PLUGIN_INSTALL_DIR} )
   install( FILES ark_dndextract.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
endif (LIBKONQ_FOUND)

