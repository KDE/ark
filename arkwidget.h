/*

 $Id $

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

#ifndef ARKWIDGET_H
#define ARKWIDGET_H

// Qt includes
#include <qdragobject.h>
#include <qlist.h>
#include <qlistview.h>
#include <qpopupmenu.h>
#include <qstring.h>
#include <qstrlist.h>
#include <qtimer.h>
#include <qwidget.h>


// KDE includes
#include <kaccel.h>
#include <kconfig.h>
#include <kpopmenu.h>
#include <ktmainwindow.h>

// ark includes
#include "ar.h"
#include "arch.h"
#include "arkdata.h"
#include "filelistview.h"
#include "tar.h"
#include "zip.h"

#define ARK_VERSION "0.5"

class ArkWidget : public KTMainWindow {

Q_OBJECT

public:
	ArkWidget( QWidget *parent=0, const char *name=0 );
	~ArkWidget();

public:
	void open_fail();
	void open_ok( QString );
	void showZip( QString name );
	void reload();
	
protected slots:
	void file_new();
	void file_newWindow();
	void file_open();
	void file_openRecent( int );
	void file_reload();
	void file_close();
	void file_quit();
	
	void edit_select();
	void edit_selectAll();
	void edit_deselectAll();
	void edit_invertSel();
	void edit_view_last_shell_output();

	void action_add();
	void action_view();
	void action_delete();
	void action_extract();
	
	void options_dirs();
	void options_keys();
	void options_general();
	void options_saveOnExit();
	void options_saveNow();

	void help();
			
	void showFavorite();
	void slotStatusBarTimeout();
	void slotSelectionChanged();
			
protected:
	static QList<ArkWidget> *windowList;
	void closeEvent( QCloseEvent * );

        // DND
        void dragEnterEvent(QDragEnterEvent* event);
        void dropEvent(QDropEvent* event);

        void createEditMenu( bool );
        void createActionMenu();
        
private:
	enum ArchType{ TAR_FORMAT, ZIP_FORMAT, AA_FORMAT, LHA_FORMAT };

	Arch *arch;

private:
	KPopupMenu *pop;
	ArkData *m_data;
	QTimer *statusBarTimer;
	KAccel *accelerators;
	FileListView *archiveContent;

	QPopupMenu *editMenu, *actionMenu, *optionsMenu, *recentPopup;
	int idActionMenu;
	int idExtract, idDelete, idAdd, idView;
	int idSelect, idSelectAll, idDeselectAll, idInvertSel;
	int idShellOutput, idSaveOnExit;

        bool archiverMode;

	void writeStatusMsg(const QString text);
	void clearStatusBar();
	
protected:	
	void clearCurrentArchive();
	
	void arkWarning(const QString& msg);
	void arkError(const QString& msg);
	
	void setupMenuBar();
	void setupStatusBar();
	void setupToolBar();
	void createRecentPopup();
	
	void newCaption(const QString& filename);
	void createFileListView();
	
	int  getArchType(QString archname);
	bool createArchive(QString name);
	bool openArchive(QString name);

	void showFile( int, int col=0 );

	void saveProperties();

};

#endif /* ARKWIDGET_H*/
