//  -*- mode: c++; c-basic-offset: 4; -*-
/*

  $Id$

  ark -- archiver for the KDE project

  Copyright (C)

  2002: Helio Chissini de Castro <helio@conectiva.com.br>
  2001: Corel Corporation (author: Michael Jarrett, michaelj@corel.com)
  1999-2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)
  1999: Francois-Xavier Duranceau duranceau@kde.org
  1997-1999: Rob Palmbos palm9744@kettering.edu

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

#define ARK_VERSION "1.9"

#include "arkwidgetbase.h"

class QWidget;
class QPoint;
class QString;
class QStringList;
class QLabel;
class QListViewItem;
class QDragMoveEvent;
class QDropEvent;
class KMainWindow;
class KPopupMenu;
class KProcess;
class KConfig;
class KURL;
class KAction;
class KRecentFilesAction;
class KToggleAction;
class KRun;
class KTempFile;

class Arch;
class ArkSettings;
class FileLVI;

class
ArkWidget : public KMainWindow, public ArkWidgetBase
{
	Q_OBJECT
	public:
		ArkWidget( QWidget *parent=0, const char *name=0 );
		virtual ~ArkWidget();
		void showZip( const QString & name );
		void reload();

		bool isEditInProgress() { return m_bEditInProgress; }

		void setExtractOnly(bool extOnly) { m_extractOnly = extOnly; }
		bool getExtractOnly() { return m_extractOnly; }

	public slots:
		void file_newWindow();
		void file_open(const KURL& url);
		void file_open(const QString &);  // opens the specified archive
		void file_save_as();
		void toggleToolBar();
		void toggleStatusBar();
		void edit_view_last_shell_output();
		void editToolbars();

	protected slots:
		void file_new();
		void file_open();
		void file_reload();
		void file_close();
		void window_close();
		void file_quit();

		void edit_select();
		void edit_selectAll();
		void edit_deselectAll();
		void edit_invertSel();

		void action_add();
		void action_add_dir();
		void action_view();
		void action_delete();
		bool action_extract();
		void slotOpenWith();
		void action_edit();
		void options_dirs();
		void options_keys();
		void options_saveNow();
		void setHeader();
		void slotSaveAsDone(KIO::Job *);
		void slotNewToolbarConfig();

		void doPopup(QListViewItem *, const QPoint &, int); // right-click menus

		void showFavorite();
		void slotSelectionChanged();
		void slotOpen(Arch *, bool, const QString &, int);
		void slotCreate(Arch *, bool, const QString &, int);
		void slotDeleteDone(bool);
		void slotExtractDone();
		void slotExtractRemoteDone(KIO::Job *job);
		void slotAddDone(bool);
		void slotEditFinished(KProcess *);
		void selectByPattern(const QString & _pattern);

	protected:
		virtual bool queryClose();
		// SM
		virtual void saveProperties( KConfig* config );
		virtual void readProperties( KConfig* config );

		// DND
		void dragMoveEvent(QDragMoveEvent *e);
		void dropEvent(QDropEvent* event);
		void dropAction(QStringList *list);
		void initialEnables();

	private: // methods
		// disabling/enabling of buttons and menu items
		void fixEnables();

		// disable all (temporarily, during operations)
		void disableAll();
		void updateStatusSelection();
		void updateStatusTotals();
		void addFile(QStringList *list);

		// make sure that str is a local file/dir
		KURL toLocalFile( QString & str);

		// ask user whether to create a real archive from a compressed file
		// returns filename if so. Otherwise, empty.
		KURL askToCreateRealArchive();
		void createRealArchive(const QString &strFilename);
		KURL getCreateFilename(const QString & _caption,
				const QString & _filter = QString::null,
				const QString & _extension = QString::null);

		// complains if the filename has capital letters or is tbz or tbz2
		bool badBzipName(const QString & _filename);
		bool reportExtractFailures(const QString & _dest, QStringList *_list);
		bool download(const KURL &, QString &);

	protected:
		void arkWarning(const QString& msg);
		void arkError(const QString& msg);

		void setupActions();
		void setupStatusBar();

		void setupMenuBar();
		void setupToolBar();

		void newCaption(const QString& filename);
		void createFileListView();

		void createArchive(const QString & name);
		void openArchive(const QString & name);

		void showFile(FileLVI *);

		void saveProperties();
	private: // data
		KAction *newWindowAction;
		KAction *newArchAction;
		KAction *openAction;
		KAction *addFileAction;
		KAction *addDirAction;
		KAction *extractAction;
		KAction *deleteAction;
		KAction *closeAction;
		KAction *reloadAction;
		KAction *selectAllAction;
		KAction *viewAction;
		KAction *helpAction;
		KAction *openWithAction;
		KAction *selectAction;
		KAction *deselectAllAction;
		KAction *invertSelectionAction;
		KAction *popupEditAction;
		KAction *editAction;
		KAction *saveAsAction;


		// the following have different enable rules from the above KActions
  		KAction *popupViewAction;
		KAction *popupOpenWithAction;

		KRecentFilesAction *recent;
		KAction *shellOutputAction;
		KToggleAction *toolbarAction;
		KToggleAction *statusbarAction;

		KPopupMenu *m_filePopup, *m_archivePopup;

		QString m_strNewArchname;

		QLabel *m_pStatusLabelSelect; // How many files are selected - label
		QLabel *m_pStatusLabelTotal;  // How many files in archive - label

		// true if user is trying to view something. For use in slotExtractDone
		bool m_bViewInProgress;
		// true if user is trying to openWith something. For use in slotExtractDone
		bool m_bOpenWithInProgress;
		// for use in slotExtractDone: the url.
		QString m_strFileToView;

		//true if user is trying to edit something. For use in slotExtractDone
		bool m_bEditInProgress;

		// true if user is trying to transform a compressed file into a real archive
		bool m_bMakeCFIntoArchiveInProgress;
		// the compressed file to be added into the new archive
		QString m_compressedFile;

		// Set to true if we are doing an "Extract to Folder"
		bool m_extractOnly;

		// Set to true if we are extracting to a remote location
		bool m_extractRemote;

		// URL to extract to.
		KURL m_extractURL;

		// if they're dragging in files, this is the temporary list for when
		// we have to create an archive:
		QStringList *m_pTempAddList;
		bool m_bDropFilesInProgress;

		KRun *m_pKRunPtr;

		KURL mSaveAsURL;

		KTempFile *mpTempFile;
		QStringList *mpDownloadedList;

		QStringList *mpAddList;
};

#endif /* ARKWIDGET_H*/

