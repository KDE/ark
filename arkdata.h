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
#define OPEN_DIR_KEY "openDir"
#define EXTRACT_DIR_KEY "extractDir"
#define ADD_DIR_KEY "addDir"
#define LAST_OPEN_DIR_KEY "lastOpenDir"
#define LAST_EXTRACT_DIR_KEY "lastExtractDir"
#define LAST_ADD_DIR_KEY "lastAddDir"

#define START_MODE_KEY "startDirMode"
#define OPEN_MODE_KEY "openDirMode"
#define EXTRACT_MODE_KEY "extractDirMode"
#define ADD_MODE_KEY "addDirMode"

#define MAX_RECENT_FILES 5


class ArkData{

public:
	ArkData();
	~ArkData();
	const QString getFilter();
	
	QString getTarCommand();
	void setTarCommand(QString cmd);
	
	QString getFavoriteDir();
	void setFavoriteDir(QString cmd);

	QString getStartDir();
        void setStartDirCfg(QString dir, int mode);

	QString getOpenDir();
	void setLastOpenDir(QString dir);
	void setOpenDirCfg(QString dir, int mode);

	QString getExtractDir();
	void setLastExtractDir(QString dir);
	void setExtractDirCfg(QString dir, int mode);

	QString getAddDir();
	void setLastAddDir(QString dir);
	void setAddDirCfg(QString dir, int mode);

	QStrList * getRecentFiles();
	void addRecentFile(const QString& filename);
	
	void setaddPath( bool b);
	bool getaddPath();

	void setonlyUpdate( bool b);
	bool getonlyUpdate();

	QStrList * getlastShellPtr();
	
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
	int startDirMode;
	
	QString openDir;
	QString lastOpenDir;
	int openDirMode;
	
	QString extractDir;
	QString lastExtractDir;
	int extractDirMode;
	
	QString addDir;
	QString lastAddDir;
	int addDirMode;
	
	enum DirPolicy{
		FAVORITE_DIR=1, FIXED_START_DIR,
		LAST_OPEN_DIR, FIXED_OPEN_DIR,
		LAST_EXTRACT_DIR, FIXED_EXTRACT_DIR,
		LAST_ADD_DIR, FIXED_ADD_DIR
	};
	
	QStrList lastShellOutput;
	
	bool addPath;
	bool onlyUpdate;

	bool contextRow;
	QStrList recentFiles;

	void readConfigFile();
	void readRecentFiles();
	void writeRecentFiles();
	void readDirectories();
	void writeDirectories();
};

#endif /* ARKDATA_H */
