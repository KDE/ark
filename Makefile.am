# set the include path for X, qt and KDE
INCLUDES= $(all_includes)
noinst_LTLIBRARIES = libark.la

SUBDIRS = pics

# The binary name. Changed from kzip to ark
bin_PROGRAMS = 	ark

# Kpart sources 
libark_la_SOURCES= arkapp.cpp extractdlg.cpp dirDlg.cpp arkwidget.cpp \
		main.cpp zip.cpp tar.cpp filelistview.cpp arch.cpp \
		selectDlg.cpp lha.cpp \
		waitDlg.cpp shellOutputDlg.cpp arksettings.cpp \
		compressedfile.cpp zoo.cpp rar.cpp ar.cpp generalOptDlg.cpp \
		ark_part.cpp arkwidgetpart.cpp arkwidgetbase.cpp \
		arkutils.cpp

# KParts
libark_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -module $(LIB_KIO)

# KParts
libark_la_LIBADD   = $(LIB_KFILE) -lkparts

# Which sources should be compiled for ark.
ark_SOURCES    = dummy.cpp

dummy.cpp:
	echo > $@

# the library search path
ark_LDFLAGS = $(KDE_RPATH) $(all_libraries)

# the libraries to link against. Be aware of the order. First the libraries,
# that depend on the following ones.
ark_LDADD   = libark.la

# this option you can leave out. Just, if you use "make dist", you need it
noinst_HEADERS = dirDlg.h arksettings.h  tar.h zip.h rar.h \
                 extractdlg.h arkwidget.h arch.h zoo.h ar.h \
		 filelistview.h lha.h compressedfile.h \
		 selectDlg.h waitDlg.h shellOutputDlg.h generalOptDlg.h \
		 ark_part.h arkwidgetpart.h arkwidgetbase.h \
		 arkutils.h

# just to make sure, automake makes them 
METASOURCES = AUTO

rcdir = $(kde_datadir)/ark
rc_DATA = arkui.rc ark_part.rc

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/ark.pot

KDE_ICON = ark

apps_DATA = ark.desktop
appsdir = $(kde_appsdir)/Utilities

parts_DATA = ark_part.desktop
partsdir = $(kde_servicesdir)

konqservice_DATA = arkservicemenu.desktop
konqservicedir = $(kde_datadir)/konqueror/servicemenus

