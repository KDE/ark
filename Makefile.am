# set the include path for X, qt and KDE
INCLUDES= $(all_includes)

SUBDIRS = pics

# The binary name. Changed from kzip to ark
bin_PROGRAMS = ark
lib_LTLIBRARIES = libarkpart.la

# Kpart sources 
libarkpart_la_SOURCES= arkapp.cpp extractdlg.cpp dirDlg.cpp arkwidget.cpp \
		zip.cpp tar.cpp filelistview.cpp arch.cpp \
		selectDlg.cpp lha.cpp shellOutputDlg.cpp arksettings.cpp \
		compressedfile.cpp zoo.cpp rar.cpp ar.cpp generalOptDlg.cpp \
		ark_part.cpp arkwidgetbase.cpp arktoplevelwindow.cpp arkfactory.cpp arkutils.cpp

# KParts
libarkpart_la_LDFLAGS = $(KDE_PLUGIN) $(all_libraries)
libarkpart_la_LIBADD  = $(LIB_KFILE) $(LIB_KPARTS)

# Which sources should be compiled for ark.
ark_SOURCES    = main.cpp
ark_LDFLAGS = $(KDE_RPATH) $(all_libraries)
ark_LDADD   = libarkpart.la

# this option you can leave out. Just, if you use "make dist", you need it
noinst_HEADERS = dirDlg.h arksettings.h  tar.h zip.h rar.h \
                 extractdlg.h arkwidget.h arch.h zoo.h ar.h \
					  filelistview.h lha.h compressedfile.h \
					  selectDlg.h shellOutputDlg.h generalOptDlg.h \
					  ark_part.h arkwidgetbase.h arktoplevelwindow.h \
					  arkfactory.h arkutils.h

# just to make sure, automake makes them 
METASOURCES = AUTO

rcdir = $(kde_datadir)/ark
rc_DATA = arkui.rc ark_part.rc ark_part_readonly.rc

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/ark.pot

KDE_ICON = ark

apps_DATA = ark.desktop
appsdir = $(kde_appsdir)/Utilities

parts_DATA = ark_part.desktop
partsdir = $(kde_servicesdir)

konqservice_DATA = arkservicemenu.desktop
konqservicedir = $(kde_datadir)/konqueror/servicemenus

