# set the include path for X, qt and KDE
AM_CPPFLAGS = -D_LARGEFILE64_SOURCE $(all_includes)

SUBDIRS = pics

# The binary name. Changed from kzip to ark
bin_PROGRAMS =
kdeinit_LTLIBRARIES = ark.la
lib_LTLIBRARIES =

ark_la_SOURCES = main.cpp arkapp.cpp mainwindow.cpp
ark_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
ark_la_LIBADD = libark_common.la $(LIB_KPARTS)
ark_la_COMPILE_FIRST = settings.h

noinst_LTLIBRARIES = libark_common.la

libark_common_la_SOURCES = settings.kcfgc archiveformatinfo.cpp
libark_common_la_LDFLAGS = $(all_libraries) -no-undefined
libark_common_la_LIBADD  = $(LIB_KFILE)

kde_module_LTLIBRARIES = libarkpart.la
libarkpart_la_LDFLAGS = $(KDE_PLUGIN) $(all_libraries)
libarkpart_la_LIBADD = libark_common.la $(LIB_KPARTS)
libarkpart_la_COMPILE_FIRST = settings.h

libarkpart_la_SOURCES = ark_part.cpp arkfactory.cpp zip.cpp tar.cpp \
                        filelistview.cpp arch.cpp lha.cpp \
                        compressedfile.cpp zoo.cpp rar.cpp \
                        ar.cpp arkutils.cpp archiveformatdlg.cpp \
                        arkwidget.cpp searchbar.cpp \
                        addition.ui extraction.ui general.ui \
                        arkviewer.cpp sevenzip.cpp extractiondialog.cpp \
                        ace.cpp tarlistingthread.cpp

METASOURCES = AUTO

rcdir = $(kde_datadir)/ark
rc_DATA = arkui.rc ark_part.rc ark_part_readonly.rc

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/ark.pot

KDE_ICON = ark

xdg_apps_DATA = ark.desktop
kde_kcfg_DATA = ark.kcfg

parts_DATA = ark_part.desktop
partsdir = $(kde_servicesdir)

#konqservice_DATA = arkservicemenu.desktop ark_directory_service.desktop
#konqservicedir = $(kde_datadir)/konqueror/servicemenus

