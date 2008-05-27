include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )
set( ark_SRCS main.cpp
              mainwindow.cpp
   )

kde4_add_executable( ark ${ark_SRCS} )

target_link_libraries( ark ${KDE4_KPARTS_LIBS} )

install( TARGETS ark  ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

install( FILES ark.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
install( FILES arkui.rc    DESTINATION ${DATA_INSTALL_DIR}/ark )
