include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )
set( ark_SRCS main.cpp
              mainwindow.cpp
   )

kde4_add_executable( arknew ${ark_SRCS} )

target_link_libraries( arknew ${KDE4_KPARTS_LIBS} )

install( TARGETS arknew  DESTINATION ${BIN_INSTALL_DIR} )

########### install files ###############

install( FILES ark.desktop DESTINATION ${XDG_APPS_DIR} )
install( FILES arkui.rc    DESTINATION ${DATA_INSTALL_DIR}/ark )
