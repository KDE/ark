#ifndef ARKDATA_H
#define ARKDATA_H

#define ARK_KEY "ark"
#define FAVORITE_KEY "ArchiveDirectory"
#define TAR_KEY "TarExe"
class ArkData{

public:
	ArkData();
	~ArkData();
	QString const getFilter();
	QString const getTarCommand();
	void setTarCommand(const QString cmd);
	QString const getFavoriteDir();
	void setFavoriteDir(const QString cmd);

private:
	KConfig *kc;
	bool opt_AddOnlyNew;
	bool opt_StoreFullPath;
	QDir *fav;
	QString fav_dir;
	QString tar_exe;
	QString tmpdir;
	bool contextRow;

	void readConfigFile();
};

#endif /* ARKDATA_H */
