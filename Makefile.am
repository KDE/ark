# set the include path for X, qt and KDE
INCLUDES= $(all_includes)
lib_LTLIBRARIES = libark.la

SUBDIRS = pics

# The binary name. Changed from kzip to ark
bin_PROGRAMS = 	ark

# Kpart sources 
libark_la_SOURCES= arkapp.cpp extractdlg.cc dirDlg.cpp arkwidget.cc \
		main.cc zip.cc tar.cc filelistview.cpp arch.cpp \
		selectDlg.cpp adddlg.cc lha.cc \
		waitDlg.cpp shellOutputDlg.cpp arksettings.cpp \
		compressedfile.cc zoo.cc rar.cc ar.cc generalOptDlg.cpp \
		ark_part.cpp arkwidgetpart.cpp arkwidgetbase.cpp kdirselectdialog.cpp \
		kdirselect.cpp

# KParts
libark_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -module $(LIB_KIO)

# KParts
libark_la_LIBADD   = $(LIB_KFILE) -lkparts

# Which sources should be compiled for ark.
ark_SOURCES    = main.cc

# the library search path
ark_LDFLAGS = $(KDE_RPATH)

# the libraries to link against. Be aware of the order. First the libraries,
# that depend on the following ones.
ark_LDADD   = libark.la

# this option you can leave out. Just, if you use "make dist", you need it
noinst_HEADERS = dirDlg.h arksettings.h  tar.h zip.h rar.h \
                 extractdlg.h arkwidget.h arch.h zoo.h ar.h \
		 filelistview.h adddlg.h lha.h compressedfile.h \
		 selectDlg.h waitDlg.h shellOutputDlg.h generalOptDlg.h \
		 ark_part.h arkwidgetpart.h arkwidgetbase.h kdirselectdialog.h \
		 kdirselect.h

# just to make sure, automake makes them 
METASOURCES =	AUTO

rcdir = $(kde_datadir)/ark
rc_DATA = arkui.rc ark_part.rc

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/ark.pot

KDE_ICON = ark

apps_DATA = ark.desktop
appsdir = $(kde_appsdir)/Utilities

parts_DATA = arkpart.desktop
partsdir = $(kde_servicesdir)

#konqservice_DATA = arkservicemenu.desktop
#konqservicedir = $(kde_datadir)/konqueror/servicemenus

