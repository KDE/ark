
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
INCLUDES= @all_includes@
# claim, which subdirectories you want to install
SUBDIRS = doc mimetypes 

####### This part is very kzip specific
# you can add here more. This one gets installed 
bin_PROGRAMS = 	kzip

# Which sources should be compiled for kzip.
kzip_SOURCES	= adddlg.cc ar.cc arch.cc extractdlg.cc karch.cc kfdird.cc \
                 kzip.cc lha.cc main.cc tar.cc zip.cc text.c kzipprocess.cc

# the library search path
kzip_LDFLAGS = @all_libraries@

# the libraries to link against. Be aware of the order. First the libraries,
# that depend on the following ones.
kzip_LDADD   = -lkfm -lkdeui -lkdecore -lqt -lX11 -lXext @LIBSOCKET@     

# this option you can leave out. Just, if you use "make dist", you need it
#noinst_HEADERS = hexfile.h hexwidget.h

# just to make sure, automake makes them 
BUILTSOURCES =	adddlg.moc arch.moc extractdlg.moc kfdird.moc kzip.moc

# if you "make distclean", this files get removed. If you want to remove
# them while "make clean", use CLEANFILES
DISTCLEANFILES = $(BUILTSOURCES)

# make messages.po. Move this one to ../po/ and "make merge" in po
messages:
	$(XGETTEXT) -C -ktranslate $(kzip_SOURCES)

# just install datas here. Use install-exec-data for scripts and etc.
# the binary itself is already installed from automake
# use mkinstalldirs, not "install -d"
# don't install a list of file. Just one file per install.
# if you have more of them, create a subdirectory with an extra Makefile 
install-data-local: 
	$(mkinstalldirs) $(APPSDIR)
	$(INSTALL_DATA) kzip.kdelnk $(APPSDIR)
	$(mkinstalldirs) $(kde_icondir)
	$(INSTALL_DATA) kzip.xpm $(kde_icondir)
	$(mkinstalldirs) $(kde_minidir)
	$(INSTALL_DATA) mini-kzip.xpm $(kde_minidir)/kzip.xpm

# remove ALL you have installed in install-data-local or install-exec-local
uninstall-local:
	-rm -f $(APPSDIR)/kzip.kdelnk
	-rm -f $(kde_icondir)/kzip.xpm
	-rm -f $(kde_minidir)/kzip.xpm

# add a dependency for every moc file to be full portable
# I've added a key binding to emacs for this. 
kzip.cc:kzip.moc
kzip.moc: kzip.h
	$(MOC) kzip.h -o kzip.moc

adddlg.cc:adddlg.moc
adddlg.moc: adddlg.h
	$(MOC) adddlg.h -o adddlg.moc

arch.cc:arch.moc
arch.moc: arch.h
	$(MOC) arch.h -o arch.moc

extractdlg.cc:extractdlg.moc
extractdlg.moc: extractdlg.h
	$(MOC) extractdlg.h -o extractdlg.moc

kfdird.cc:kfdird.moc
kfdird.moc: kfdird.h
	$(MOC) kfdird.h -o kfdird.moc

kzipprocess.cc:kzipprocess.moc
kzipprocess.moc: kzipprocess.h
	$(MOC) kzipprocess.h -o kzipprocess.moc