//  -*-C++-*-           emacs magic for .h files
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
#include "arksettings.h"
#include "filelistview.h"

#define ARK_VERSION "1.9"

//
//
// This file contains the class definitions for CArkWidget (the main
// widget for the app), ArkListView (the listview widget tailored to
// be a drop-site), and MyListViewItem (the listview item tailored to
// sorting sizes numerically instead of asciibetically)
//

class Viewer;
class Arch;

enum ArchType {UNKNOWN_FORMAT, ZIP_FORMAT, TAR_FORMAT, AA_FORMAT,
	       LHA_FORMAT, RAR_FORMAT, ZOO_FORMAT};

class ArkWidget : public KTMainWindow 
{
  Q_OBJECT
public:
  ArkWidget( QWidget *parent=0, const char *name=0 );
  ~ArkWidget();
  bool isArchiveOpen() { return m_bIsArchiveOpen; }
  QString getArchName() { return m_strArchName; }
  void showZip( QString name );
  void reload();
  FileListView *fileList() const { return archiveContent; };

  void remove();
  void listingAdd(QStringList *_entries);
  void setHeaders(QStringList *_headers,
		  int * _rightAlignCols, int _numColsToAlignRight);

public slots:    
void file_newWindow();
  void file_open(const QString &);  // opens the specified archive
  //    void open_fail();
  //    void open_ok( QString );
    
protected slots:
  void file_new();
  void file_open();
  void file_openRecent( int );
  void file_reload();
  void file_close();
  void window_close();
  void file_quit();
	
  void edit_select();
  void edit_selectAll();
  void edit_deselectAll();
  void edit_invertSel();
  void edit_view_last_shell_output();
    
  void action_add();
  void action_add_dir();
  void action_view();
  void action_delete();
  void action_extract();
	
  void options_dirs();
  void options_keys();
  void options_general();
  void options_saveOnExit();
  void options_saveNow();

  void help();
		
  void doPopup(QListViewItem *, const QPoint &, int); // right-click menus
	
  void showFavorite();
  //    void slotStatusBarTimeout();
  void slotSelectionChanged();
  void slotOpen(Arch *, bool, const QString &, int);
  void slotCreate(Arch *, bool, const QString &, int);
  void slotDeleteDone(bool);
  void slotExtractDone();
  void slotAddDone(bool);

  void selectByPattern(const QString & _pattern);

protected:
  void closeEvent( QCloseEvent * );

  // DND
  void dragMoveEvent(QDragMoveEvent *e);
  //  void dragEnterEvent(QDragEnterEvent* event);
  void dropEvent(QDropEvent* event);
  void dropAction(QStringList *list);

  void createActionMenu( int );
  void initialEnables();

private: // methods
  // disabling/enabling of buttons and menu items
  void fixEnables();
    
  void updateStatusSelection();
  void updateStatusTotals();
       
  void addFile(QStringList *list);
private:

  Arch *arch;

private:
  KPopupMenu *m_filePopup, *m_archivePopup;
  ArkSettings *m_settings;  // each arkwidget has its own settings

  //    QTimer *statusBarTimer;
  KAccel *accelerators;
  FileListView *archiveContent;

  QString m_strArchName;

  QPopupMenu *fileMenu, *editMenu, *actionMenu, *optionsMenu;
  QPopupMenu *recentPopup;

  //  int  idActionMenu, idEditMenu;
  int idExtract, idDelete, idAdd, idView;
  //  int idSaveOnExit;

  bool archiverMode;

  //    void writeStatusMsg(const QString text);
  //  void clearStatusBar();
  void createEditMenu();
	
protected:	
  void arkWarning(const QString& msg);
  void arkError(const QString& msg);
	
  void setupMenuBar();
  void setupStatusBar();
  void setupToolBar();
  void createRecentPopup();
	
  void newCaption(const QString& filename);
  void createFileListView();
	
  ArchType getArchType(QString archname);
  void createArchive(QString name);
  void openArchive(QString name);

  void showFile(FileLVI *);

  void saveProperties();
private:
  // totals for status bar:
  int m_nSizeOfFiles;
  int m_nSizeOfSelectedFiles;
  int m_nNumFiles;
  int m_nNumSelectedFiles;

  // some informational bools
  bool m_bIsArchiveOpen;
  bool m_bIsSimpleCompressedFile;
  bool m_bDropSourceIsSelf; // no dragging from and dropping on myself

  Viewer *m_viewer; // for separating gui - archives know viewer not arkwidget

 // true if user is trying to view something. For use in slotExtractDone
  bool m_bViewInProgress;
  QString strFileToView;
};

// menu ids
enum { eMFile, eMAction, eMEdit, eMOptions, eMHelp } ;

// toolbar buttons
enum { eNew, eOpen, eAddFile, eAddDir, eExtract, eDelete,
       eSelectAll, eView, eOptions, eHelp };
// popup menu items
enum { eMNew, eMOpen, eMReload, eMClose, eMWindow, eMExit, eMAddFile,
       eMAddDir, eMDelete, eMExtract, eMView, eMSelectAll, eMRename,
       eMSaveOnExit, eMSelect, eMDeselectAll, eMInvertSel};


// status item numbers

enum { eSelectedStatusLabel, eStatusLabelSeparator, eNumFilesStatusLabel,
       eStatusDummy };
#endif /* ARKWIDGET_H*/
