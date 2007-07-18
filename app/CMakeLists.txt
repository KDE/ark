include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )
set( ark_SRCS main.cpp
              mainwindow.cpp
   )

kde4_add_executable( ark ${ark_SRCS} )

target_link_libraries( ark ${KDE4_KPARTS_LIBS} )

install( TARGETS ark  DESTINATION ${BIN_INSTALL_DIR} )

########### install files ###############

install( FILES ark.desktop DESTINATION ${XDG_APPS_DIR} )
install( FILES arkui.rc    DESTINATION ${DATA_INSTALL_DIR}/ark )
