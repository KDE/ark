/*

 $Id$

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

	//	const QString getFilter();
	
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

	QStringList * getRecentFiles();
	void addRecentFile(const QString& filename);

	void setSaveOnExitChecked( bool );
	bool isSaveOnExitChecked();
	
	void setaddPath( bool b);
	bool getaddPath();

	void setonlyUpdate( bool b);
	bool getonlyUpdate();

	void setSelectRegExp(const QString& _exp);
	QString getSelectRegExp() const;

	void appendShellOutputData( const char * );
	void clearShellOutput();
	QString * getLastShellOutput() const;

	void setZipExtractOverwrite( bool );
	bool getZipExtractOverwrite();
	
	void setZipExtractJunkPaths( bool );
	bool getZipExtractJunkPaths();
	
	void setZipExtractLowerCase( bool );
	bool getZipExtractLowerCase();

	void setZipAddRecurseDirs( bool );
	bool getZipAddRecurseDirs();

	void setZipAddJunkDirs( bool );
	bool getZipAddJunkDirs();

	void setZipAddMSDOS( bool );
	bool getZipAddMSDOS();

	void setZipAddConvertLF( bool );
	bool getZipAddConvertLF();
			
	void setTmpDir( QString );
	QString getTmpDir() const;			
	void writeConfiguration();
	void writeConfigurationNow();
	void readConfiguration();
	
	KConfig * getKConfig() { return kc; };
	
protected:
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
	
	QString * m_lastShellOutput;
	
	bool m_saveOnExit;
	QString m_regExp;

	bool addPath;
	bool onlyUpdate;

	bool contextRow;
	QStringList recentFiles;
	
	bool m_zipExtractOverwrite;
	bool m_zipExtractJunkPaths;
	bool m_zipExtractLowerCase;
	
	bool m_zipAddRecurseDirs;
	bool m_zipAddJunkDirs;
	bool m_zipAddMSDOS;
	bool m_zipAddConvertLF;

	QString m_tmpDir;
	
	void readRecentFiles();
	void writeRecentFiles();
	
	void readDirectories();
	void writeDirectories();
	
	void readZipProperties();
	void writeZipProperties();
};

#endif /* ARKDATA_H */
