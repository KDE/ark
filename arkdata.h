/*

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

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
	
	enum DirPolicy{
		FAVORITE_DIR=1, FIXED_START_DIR,
		LAST_OPEN_DIR, FIXED_OPEN_DIR,
		LAST_EXTRACT_DIR, FIXED_EXTRACT_DIR,
		LAST_ADD_DIR, FIXED_ADD_DIR
	};

	const QString getFilter();
	
	QString getTarCommand() const;
	void setTarCommand(const QString& cmd);
	
	QString getFavoriteDir() const;
	void setFavoriteDir(const QString& cmd);

	QString getStartDir() const;
	QString getFixedStartDir() const;
	int getStartDirMode() const;
        void setStartDirCfg(const QString& dir, int mode);

	QString getOpenDir() const;
	QString getFixedOpenDir() const;
	int getOpenDirMode() const;
	void setLastOpenDir(const QString& dir);
	void setOpenDirCfg(const QString& dir, int mode);

	QString getExtractDir();
	QString getFixedExtractDir() const;
	int getExtractDirMode() const;
	void setLastExtractDir(const QString& dir);
	void setExtractDirCfg(const QString& dir, int mode);

	QString getAddDir();
	QString getFixedAddDir() const;
	int getAddDirMode() const;
	void setLastAddDir(const QString& dir);
	void setAddDirCfg(const QString& dir, int mode);

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
