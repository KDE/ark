
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
# claim, which subdirectories you want to install
SUBDIRS = doc

# The binary name. Changed from kzip to ark
bin_PROGRAMS = 	ark

# Which sources should be compiled for ark.
ark_SOURCES	= adddlg.cc ar.cc arch.cc extractdlg.cc karchive.cc  \
                 arkwidget.cc lha.cc main.cc tar.cc zip.cc arkprocess.cc text.c

# the library search path
ark_LDFLAGS = $(all_libraries) $(KDE_RPATH)

# the libraries to link against. Be aware of the order. First the libraries,
# that depend on the following ones.
ark_LDADD   = $(LIB_KFILE)

# this option you can leave out. Just, if you use "make dist", you need it
noinst_HEADERS = adddlg.h errors.h lha.h zip.h \
                 ar.h extractdlg.h arkwidget.h tar.h arch.h karchive.h arkprocess.h \
		 text.h 

# just to make sure, automake makes them 
METASOURCES =	adddlg.moc arch.moc extractdlg.moc arkwidget.moc

messages:
	$(XGETTEXT) -C -ktranslate -ki18n -x $(includedir)/kde.pot $(ark_SOURCES) &&  mv messages.po ../po/ark.pot

kde_icon_DATA = ark.xpm

apps_DATA = ark.kdelnk
appsdir = $(kde_appsdir)/Utilities

# just install datas here. Use install-exec-data for scripts and etc.
# the binary itself is already installed from automake
# use mkinstalldirs, not "install -d"
# don't install a list of file. Just one file per install.
# if you have more of them, create a subdirectory with an extra Makefile 
install-data-local: 
	$(mkinstalldirs) $(kde_minidir)
	$(INSTALL_DATA) $(srcdir)/mini-ark.xpm $(kde_minidir)/ark.xpm
	$(mkinstalldirs) $(kde_minidir)/../large
	$(INSTALL_DATA) $(srcdir)/large-ark.xpm $(kde_minidir)/../large/ark.xpm
# Get rid of kzip
	-rm -f $(kde_bindir)/kzip
	-rm -f $(appsdir)/kzip.kdelnk
	-rm -f $(kde_icondir)/kzip.xpm
	-rm -f $(kde_minidir)/kzip.xpm

# remove ALL you have installed in install-data-local or install-exec-local
uninstall-local:
	-rm -f $(kde_minidir)/ark.xpm
	-rm -f $(kde_minidir)/../large/ark.xpm

