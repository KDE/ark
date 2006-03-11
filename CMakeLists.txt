kde4_header()

add_subdirectory( pics ) 

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )

set(libark_common_SRCS archiveformatinfo.cpp )
kde4_add_kcfg_files(libark_common_SRCS settings.kcfgc)

########### next target ###############

set(ark_KDEINIT_SRCS main.cpp arkapp.cpp mainwindow.cpp ${libark_common_SRCS})

kde4_automoc(${ark_KDEINIT_SRCS})

kde4_add_kdeinit_executable( ark ${ark_KDEINIT_SRCS})

target_link_libraries(kdeinit_ark  ${LIB_KDE3SUPPORT} )

install_targets(${LIB_INSTALL_DIR} kdeinit_ark )

target_link_libraries( ark kdeinit_ark )
install_targets(/bin ark )

########### next target ###############


########### next target ###############

set(arkpart_PART_SRCS ${libark_common_SRCS} 
   ark_part.cpp 
   arkfactory.cpp 
   zip.cpp 
   tar.cpp 
   filelistview.cpp 
   archive.cpp 
   lha.cpp 
   compressedfile.cpp 
   zoo.cpp 
   rar.cpp 
   ar.cpp 
   arkutils.cpp 
   archiveformatdlg.cpp 
   arkwidget.cpp 
   searchbar.cpp 
   arkviewer.cpp 
   sevenzip.cpp 
   extractiondialog.cpp 
   ace.cpp )

kde4_automoc(${arkpart_PART_SRCS})

kde4_add_ui3_files(arkpart_PART_SRCS general.ui )

kde4_add_plugin(arkpart WITH_PREFIX ${arkpart_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} arkpart )

target_link_libraries(arkpart  ${LIB_KDE3SUPPORT} )

install_targets(${LIB_INSTALL_DIR}/kde4 arkpart )


########### install files ###############

install_files( ${XDG_APPS_DIR} FILES ark.desktop )
install_files( ${KCFG_INSTALL_DIR} FILES ark.kcfg )
install_files( ${SERVICES_INSTALL_DIR} FILES ark_part.desktop )
install_files( ${DATA_INSTALL_DIR}/ark FILES arkui.rc ark_part.rc ark_part_readonly.rc )

kde4_install_icons( ${ICON_INSTALL_DIR}  hicolor )

kde4_footer()



#original Makefile.am contents follow:

## set the include path for X, qt and KDE
#AM_CPPFLAGS = -D_LARGEFILE64_SOURCE $(all_includes)
#
#SUBDIRS = pics
#
## The binary name. Changed from kzip to ark
#bin_PROGRAMS =
#kdeinit_LTLIBRARIES = ark.la
#lib_LTLIBRARIES =
#
#ark_la_SOURCES = main.cpp arkapp.cpp mainwindow.cpp
#ark_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#ark_la_LIBADD = libark_common.la $(LIB_KPARTS) $(LIB_KDE3SUPPORT)
#ark_la_COMPILE_FIRST = settings.h
#
#noinst_LTLIBRARIES = libark_common.la
#
#libark_common_la_SOURCES = settings.kcfgc archiveformatinfo.cpp
#libark_common_la_LDFLAGS = $(all_libraries) -no-undefined
#libark_common_la_LIBADD  = $(LIB_KFILE) $(LIB_KDE3SUPPORT)
#
#kde_module_LTLIBRARIES = libarkpart.la
#libarkpart_la_LDFLAGS = $(KDE_PLUGIN) $(all_libraries)
#libarkpart_la_LIBADD = libark_common.la $(LIB_KPARTS) $(LIB_KDE3SUPPORT)
#libarkpart_la_COMPILE_FIRST = settings.h
#
#libarkpart_la_SOURCES = ark_part.cpp arkfactory.cpp zip.cpp tar.cpp \
#                        filelistview.cpp archive.cpp lha.cpp \
#                        compressedfile.cpp zoo.cpp rar.cpp \
#                        ar.cpp arkutils.cpp archiveformatdlg.cpp \
#                        arkwidget.cpp searchbar.cpp \
#                        general.ui \
#                        arkviewer.cpp sevenzip.cpp extractiondialog.cpp \
#                        ace.cpp
#
#METASOURCES = AUTO
#
#rcdir = $(kde_datadir)/ark
#rc_DATA = arkui.rc ark_part.rc ark_part_readonly.rc
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp -o $(podir)/ark.pot
#
#KDE_ICON = ark
#
#xdg_apps_DATA = ark.desktop
#kde_kcfg_DATA = ark.kcfg
#
#parts_DATA = ark_part.desktop
#partsdir = $(kde_servicesdir)
#
##konqservice_DATA = arkservicemenu.desktop ark_directory_service.desktop
##konqservicedir = $(kde_datadir)/konqueror/servicemenus
#
