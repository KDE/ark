# set the include path for X, qt and KDE
INCLUDES= $(all_includes)

SUBDIRS = pics

# The binary name. Changed from kzip to ark
bin_PROGRAMS = 	ark

# Which sources should be compiled for ark.
ark_SOURCES	= arkapp.cpp extractdlg.cc dirDlg.cpp arkwidget.cc \
		main.cc zip.cc tar.cc filelistview.cpp arch.cpp \
		selectDlg.cpp adddlg.cc lha.cc \
		waitDlg.cpp shellOutputDlg.cpp arksettings.cpp \
		compressedfile.cc zoo.cc rar.cc ar.cc ktar.cpp

# the library search path
ark_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_KIO)

# the libraries to link against. Be aware of the order. First the libraries,
# that depend on the following ones.
ark_LDADD   = $(LIB_KFILE)

# this option you can leave out. Just, if you use "make dist", you need it
noinst_HEADERS = dirDlg.h arksettings.h  tar.h zip.h rar.h \
                 extractdlg.h arkwidget.h arch.h zoo.h ar.h \
		 filelistview.h adddlg.h lha.h compressedfile.h \
		 selectDlg.h waitDlg.h shellOutputDlg.h

# just to make sure, automake makes them 
METASOURCES =	AUTO

rcdir = $(kde_datadir)/ark
rc_DATA = arkui.rc

messages: rc.cpp
	$(XGETTEXT) rc.cpp $(ark_SOURCES) common_texts.cpp -o $(podir)/ark.pot

KDE_ICON = ark

apps_DATA = ark.desktop
appsdir = $(kde_appsdir)/Utilities

