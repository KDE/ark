
# this 10 paths are KDE specific. Use them:
# kde_htmldir       Where your docs should go to. (contains lang subdirs)
# kde_appsdir       Where your application file (.kdelnk) should go to. 
# kde_icondir       Where your icon should go to.
# kde_minidir       Where your mini icon should go to.
# kde_datadir       Where you install application data. (Use a subdir)
# kde_locale        Where translation files should go to.(contains lang subdirs)
# kde_cgidir        Where cgi-bin executables should go to.
# kde_confdir       Where config files should go to.
# kde_mimedir       Where mimetypes should go to.
# kde_toolbardir    Where general toolbar icons should go to.
# kde_wallpaperdir  Where general wallpapers should go to.

# set the include path for X, qt and KDE
INCLUDES= $(all_includes)

SUBDIRS = pics

# The binary name. Changed from kzip to ark
bin_PROGRAMS = 	ark

# Which sources should be compiled for ark.
ark_SOURCES	= arkapp.cpp extractdlg.cc dirDlg.cpp arkwidget.cc \
		main.cc zip.cc tar.cc filelistview.cpp arch.cpp \
		generalOptDlg.cpp selectDlg.cpp adddlg.cc lha.cc \
		waitDlg.cpp shellOutputDlg.cpp arksettings.cpp \
		compressedfile.cc

# the library search path
ark_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_KIO)

# the libraries to link against. Be aware of the order. First the libraries,
# that depend on the following ones.
ark_LDADD   = $(LIB_KFILE)

# this option you can leave out. Just, if you use "make dist", you need it
noinst_HEADERS = dirDlg.h arksettings.h  tar.h zip.h \
                 extractdlg.h arkwidget.h arch.h \
		 filelistview.h adddlg.h lha.h compressedfile.h
		 generalOptDlg.h selectDlg.h waitDlg.h shellOutputDlg.h

# just to make sure, automake makes them 
METASOURCES =	AUTO

messages:
	$(XGETTEXT) $(ark_SOURCES) -o $(podir)/ark.pot

KDE_ICON = ark

apps_DATA = ark.desktop
appsdir = $(kde_appsdir)/Utilities

