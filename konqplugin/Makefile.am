INCLUDES    = $(all_includes)
METASOURCES = AUTO

KDE_ICON = ark

# Install this plugin in the KDE modules directory
kde_module_LTLIBRARIES = libarkplugin.la

libarkplugin_la_SOURCES = arkplugin.cpp
libarkplugin_la_LIBADD = -lkonq
libarkplugin_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)

pluginsdir   = $(kde_servicesdir)
plugins_DATA = ark_plugin.desktop

appsdir = $(kde_appsdir)/.hidden
apps_DATA = arkplugin.desktop

messages: rc.cpp
	$(XGETTEXT) *.cpp *.h -o $(podir)/ark_plugin.pot
