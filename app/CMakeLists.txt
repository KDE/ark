add_subdirectory(icons)

set(ark_SRCS
    batchextract.cpp
    main.cpp
    mainwindow.cpp
   )

# For Mac and Windows.
kde4_add_app_icon(ark_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/icons/hi*-apps-ark.png")

kde4_add_executable( ark ${ark_SRCS} )

target_link_libraries( ark kerfuffle ${KDE4_KFILE_LIBS} ${KDE4_KPARTS_LIBS} )

configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/ark.desktop.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/ark.desktop
)

configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/ark_dndextract.desktop.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/ark_dndextract.desktop
)

configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/ark_servicemenu.desktop.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/ark_servicemenu.desktop
)

install( TARGETS ark  ${INSTALL_TARGETS_DEFAULT_ARGS} )
install(FILES ark_addtoservicemenu.desktop ${CMAKE_CURRENT_BINARY_DIR}/ark_servicemenu.desktop DESTINATION ${SERVICES_INSTALL_DIR}/ServiceMenus)
install( PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/ark.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
install( FILES ${CMAKE_CURRENT_SOURCE_DIR}/ark.appdata.xml DESTINATION share/appdata )
install( FILES arkui.rc    DESTINATION ${DATA_INSTALL_DIR}/ark )

########### konqueror ark_extract_here plugin ###############
macro_optional_find_package( LibKonq )
macro_log_feature( LIBKONQ_FOUND "LIBKONQ" "libkonq library" "kdebase" FALSE "" "Need to integrate in konqueror" )


if (LIBKONQ_FOUND)
   set(extracthere_SRCS batchextract.cpp extractHereDndPlugin.cpp)
   kde4_add_plugin(extracthere WITH_PREFIX ${extracthere_SRCS})
   target_link_libraries(extracthere kerfuffle ${KDE4_KDECORE_LIBS} ${KDE4_KPARTS_LIBS} ${KDE4_KFILE_LIBS} ${LIBKONQ_LIBRARY} )
   include_directories(${LIBKONQ_INCLUDE_DIR})
   install( TARGETS extracthere DESTINATION ${PLUGIN_INSTALL_DIR} )
   install( FILES ${CMAKE_CURRENT_BINARY_DIR}/ark_dndextract.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
endif (LIBKONQ_FOUND)

