
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

# The binary name. Changed from kzip to ark
bin_PROGRAMS = 	ark

# Which sources should be compiled for ark.
ark_SOURCES	= extractdlg.cc dirDlg.cpp zipAddDlg.cpp \
                 arkwidget.cc main.cc zip.cc arch.cpp \
		arkdata.cpp filelistview.cpp zipExtractDlg.cpp \
		generalOptDlg.cpp selectDlg.cpp waitDlg.cpp shellOutputDlg.cpp


# the library search path
ark_LDFLAGS = $(all_libraries) $(KDE_RPATH)

# the libraries to link against. Be aware of the order. First the libraries,
# that depend on the following ones.
ark_LDADD   = $(LIB_KFILE)

# this option you can leave out. Just, if you use "make dist", you need it
noinst_HEADERS = dirDlg.h zipAddDlg.h \
                 extractdlg.h arkwidget.h arch.h \
		 arkdata.h filelistview.h zipExtractDlg.h \
		 generalOptDlg.h selectDlg.h waitDlg.h shellOutputDlg.h

# just to make sure, automake makes them 
METASOURCES =	extractdlg.moc arkwidget.moc filelistview.moc arch.moc \
		dirDlg.moc zipExtractDlg.moc generalOptDlg.moc selectDlg.moc \
		zip.moc waitDlg.moc shellOutputDlg.moc zipAddDlg.moc

messages:
	$(XGETTEXT) -C -ki18n -x $(includedir)/kde.pot $(ark_SOURCES) &&  mv messages.po ../po/ark.pot

KDE_ICON = ark

apps_DATA = ark.desktop
appsdir = $(kde_appsdir)/Programs/Utilities

