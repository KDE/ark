
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

# just set the variable
APPSDIR = $(kde_appsdir)/Utilities
MIMEAPPDIR = $(kde_mimedir)/application
# set the include path for X, qt and KDE
INCLUDES= $(all_includes)
# claim, which subdirectories you want to install
SUBDIRS = doc mimetypes

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
BUILTSOURCES =	adddlg.moc arch.moc extractdlg.moc arkwidget.moc

# if you "make distclean", this files get removed. If you want to remove
# them while "make clean", use CLEANFILES
DISTCLEANFILES = $(BUILTSOURCES)

messages:
	$(XGETTEXT) -C -ktranslate -ki18n -x $(includedir)/kde.pot $(ark_SOURCES) &&  mv messages.po ../po/ark.pot

# just install datas here. Use install-exec-data for scripts and etc.
# the binary itself is already installed from automake
# use mkinstalldirs, not "install -d"
# don't install a list of file. Just one file per install.
# if you have more of them, create a subdirectory with an extra Makefile 
install-data-local: 
	$(mkinstalldirs) $(APPSDIR)
	$(INSTALL_DATA) ark.kdelnk $(APPSDIR)
	$(mkinstalldirs) $(kde_icondir)
	$(INSTALL_DATA) ark.xpm $(kde_icondir)
	$(mkinstalldirs) $(kde_minidir)
	$(INSTALL_DATA) mini-ark.xpm $(kde_minidir)/ark.xpm

# remove ALL you have installed in install-data-local or install-exec-local
uninstall-local:
	-rm -f $(APPSDIR)/ark.kdelnk
	-rm -f $(kde_icondir)/ark.xpm
	-rm -f $(kde_minidir)/ark.xpm

# add a dependency for every moc file to be full portable
# I've added a key binding to emacs for this. 
arkwidget.cc:arkwidget.moc
arkwidget.moc: arkwidget.h
	$(MOC) arkwidget.h -o arkwidget.moc

adddlg.cc:adddlg.moc
adddlg.moc: adddlg.h
	$(MOC) adddlg.h -o adddlg.moc

arch.cc:arch.moc
arch.moc: arch.h
	$(MOC) arch.h -o arch.moc

extractdlg.cc:extractdlg.moc
extractdlg.moc: extractdlg.h
	$(MOC) extractdlg.h -o extractdlg.moc

