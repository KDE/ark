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

#define START_DIR_KEY "startDir"
#define OPEN_DIR_KEY "lastOpenDir"
#define EXTRACT_DIR_KEY "lastExtractDir"
#define ADD_DIR_KEY "lastAddDir"

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

	const QString getStartDir();
	void setStartDir(const QString dir);

	const QString getOpenDir();
	void setOpenDir(const QString dir);

	const QString getExtractDir();
	void setExtractDir(const QString dir);

	const QString getAddDir();
	void setAddDir(const QString dir);

	QStrList * getRecentFiles();
	void addRecentFile(const QString& filename);
	
	void setaddPath( bool& b);
	bool getaddPath();

	void setonlyUpdate( bool& b);
	bool getonlyUpdate();

	void writeConfiguration();
	
	KConfig * getKConfig() { return kc; };
private:
	KConfig *kc;
	bool opt_AddOnlyNew;
	bool opt_StoreFullPath;

	QString favoriteDir;
	QString tar_exe;

    // Directories options
	QString tmpdir;
	QString startDir;
	QString openDir;
	QString extractDir;
	QString addDir;
	
	bool addPath;
	bool onlyUpdate;

	bool contextRow;
	QStrList recentFiles;

	void readConfigFile();
	void readRecentFiles();
	void writeRecentFiles();
};

#endif /* ARKDATA_H */
