//  -*- mode: c++; c-basic-offset: 4; -*-
/*
 
  ark -- archiver for the KDE project
 
  Copyright (C)
 
  2003: Georg Robbers <Georg.Robbers@urz.uni-hd.de>
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
#include "arktoplevelwindow.h"

class QWidget;
class QPoint;
class QString;
class QStringList;
class QLabel;
class QListViewItem;
class QDragMoveEvent;
class QDropEvent;
class KPopupMenu;
class KProcess;
class KConfig;
class KURL;
class KRun;
class KTempFile;

class Arch;
class ArkSettings;
class FileLVI;


class ArkWidget : public QWidget, public ArkWidgetBase
{
    Q_OBJECT
public:
    ArkWidget( QWidget *parent=0, const char *name=0 );
    virtual ~ArkWidget();

    void setExtractOnly(bool extOnly) { m_extractOnly = extOnly; }
    bool getExtractOnly() { return m_extractOnly; }
    ArkSettings *settings() { return m_settings; }
    bool allowedArchiveName( const KURL & u );
    bool file_save_as( const KURL & u );
    KURL getSaveAsFileName();
    void setOpenAsMimeType( const QString & mimeType );
    QString & openAsMimeType(){ return m_openAsMimeType; };
    void prepareViewFiles( const QStringList & fileList );

public slots:
    void file_open(const KURL& url);
    void edit_view_last_shell_output();
    void file_close();
    void file_new();
    void options_dirs();
    void options_saveNow();

protected slots:
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
    void slotSaveAsDone(KIO::Job *);

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
signals:
    void request_file_quit();
    void fixActions();
    void disableAllActions();
    void signalFilePopup( const QPoint & pPoint );
    void signalArchivePopup( const QPoint & pPoint );
    void setStatusBarText( const QString & text );
    void setStatusBarSelectedFiles( const QString & text );
    void removeRecentURL( const QString & url );
    void addRecentURL( const QString & url );
    void setWindowCaption( const QString &caption );
    void removeOpenArk( const KURL & );
    void addOpenArk( const KURL & );

protected:

    // DND
    void dragMoveEvent(QDragMoveEvent *e);
    void dropEvent(QDropEvent* event);
    void dropAction(QStringList & list);

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
    Arch * getNewArchive( const QString & _fileName );
    void createRealArchive( const QString &strFilename,
                            const QStringList & filesToAdd = QStringList() );
    KURL getCreateFilename( const QString & _caption,
                            const QString & _filter = QString::null,
                            const QString & _extension = QString::null,
                            bool allowCompressed = true );

    bool reportExtractFailures(const QString & _dest, QStringList *_list);

    void extractOnlyOpenDone();
    void extractRemoteInitiateMoving();
    void editStart();

private slots:
    void startDrag( const QStringList & fileList );
    void startDragSlotExtractDone( bool );
    void editSlotExtractDone();
    void editSlotAddDone( bool success );
    void viewSlotExtractDone();
    void openWithSlotExtractDone();
    void createRealArchiveSlotCreate( Arch * newArch, bool success,
                                      const QString & fileName, int nbr );
    void createRealArchiveSlotAddDone( bool success );
    void createRealArchiveSlotAddFilesDone( bool success );

protected:
    void arkWarning(const QString& msg);
    void arkError(const QString& msg);

    void newCaption(const QString& filename);
    void createFileListView();

    void createArchive(const QString & name);
    void openArchive(const QString & name);

    void showCurrentFile();

private: // data

    // for use in the edit methods: the url.
    QString m_strFileToView;

    // the compressed file to be added into the new archive
    QString m_compressedFile;

    // Set to true if we are doing an "Extract to Folder"
    bool m_extractOnly;

    // Set to true if we are extracting to a remote location
    bool m_extractRemote;

    // URL to extract to.
    KURL m_extractURL;

    // the mimetype the user wants to open this archive as
    QString m_openAsMimeType;

    // if they're dragging in files, this is the temporary list for when
    // we have to create an archive:
    QStringList *m_pTempAddList;

    KRun *m_pKRunPtr;

    QStringList *mpDownloadedList;

};

#endif /* ARKWIDGET_H*/

