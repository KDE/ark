# set the include path for X, qt and KDE
AM_CPPFLAGS = $(all_includes)

SUBDIRS = pics konqplugin

# The binary name. Changed from kzip to ark
bin_PROGRAMS = 
kdeinit_LTLIBRARIES = ark.la
lib_LTLIBRARIES = 

ark_la_SOURCES = main.cpp arkapp.cpp arktoplevelwindow.cpp
ark_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
ark_la_LIBADD = libark_common.la $(LIB_KPARTS)

noinst_LTLIBRARIES = libark_common.la

libark_common_la_SOURCES = archiveformatinfo.cpp arksettings.cpp
libark_common_la_LDFLAGS = $(all_libraries) -no-undefined
libark_common_la_LIBADD  = $(LIB_KFILE)

kde_module_LTLIBRARIES = libarkpart.la
libarkpart_la_LDFLAGS = $(KDE_PLUGIN) $(all_libraries)
libarkpart_la_LIBADD = libark_common.la $(LIB_KPARTS)

libarkpart_la_SOURCES = ark_part.cpp arkfactory.cpp extractdlg.cpp dirDlg.cpp \
                zip.cpp tar.cpp filelistview.cpp arch.cpp selectDlg.cpp \
                lha.cpp shellOutputDlg.cpp compressedfile.cpp \
                zoo.cpp rar.cpp ar.cpp generalOptDlg.cpp arkwidgetbase.cpp \
                arkutils.cpp archiveformatdlg.cpp arkwidget.cpp searchbar.cpp


# this option you can leave out. Just, if you use "make dist", you need it
noinst_HEADERS = dirDlg.h arksettings.h  tar.h zip.h rar.h \
                 extractdlg.h arkwidget.h arch.h zoo.h ar.h \
                 filelistview.h lha.h compressedfile.h \
                 selectDlg.h shellOutputDlg.h generalOptDlg.h \
                 ark_part.h arkwidgetbase.h arktoplevelwindow.h \
                 arkfactory.h arkutils.h archiveformatinfo.h \
                 archiveformatdlg.h searchbar.h

METASOURCES = AUTO

rcdir = $(kde_datadir)/ark
rc_DATA = arkui.rc ark_part.rc ark_part_readonly.rc

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/ark.pot

KDE_ICON = ark

xdg_apps_DATA = ark.desktop

parts_DATA = ark_part.desktop
partsdir = $(kde_servicesdir)

#konqservice_DATA = arkservicemenu.desktop ark_directory_service.desktop
#konqservicedir = $(kde_datadir)/konqueror/servicemenus

