#ifndef ARKDATA_H
#define ARKDATA_H

// Qt includes
#include <qstrlist.h>
#include <qstring.h>

// KDE includes
#include <kconfig.h>

// Key names in the arkrc config file
#define ARK_KEY "ark"
#define FAVORITE_KEY "ArchiveDirectory"
#define TAR_KEY "TarExe"
#define RECENT_KEY "Recent"

#define MAX_RECENT_FILES 5

class ArkData{

public:
	ArkData();
	~ArkData();
	const QString getFilter();
	
	const QString getTarCommand();
	void setTarCommand(const QString cmd);
	
	const QString getFavoriteDir();
	void setFavoriteDir(const QString cmd);
	
	QStrList * getRecentFiles();
	void addRecentFile(const QString& filename);

	void writeConfiguration();
	
	KConfig * getKConfig() { return kc; };
private:
	KConfig *kc;
	bool opt_AddOnlyNew;
	bool opt_StoreFullPath;
//	QDir *fav;
	QString fav_dir;
	QString tar_exe;
	QString tmpdir;
	bool contextRow;
	QStrList recentFiles;
	void readConfigFile();
	void readRecentFiles();
	void writeRecentFiles();
};

#endif /* ARKDATA_H */
