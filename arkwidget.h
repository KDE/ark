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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#ifndef ARKWIDGET_H
#define ARKWIDGET_H

#include <kio/job.h>
#include <ktempdir.h>
#include <kvbox.h>

#include <QLabel>
#include <QDragMoveEvent>
#include <QDropEvent>

#include "archive.h"
#include "ui_general.h"

class K3ListViewSearchLine;
class QPoint;
class QString;
class QStringList;
class QLabel;
class Q3ListViewItem;
class QDragMoveEvent;
class QDropEvent;

class KMenu;
class K3Process;
class KUrl;
class KRun;
class KTemporaryFile;
class KTempDir;
class KToolBar;

class FileListView;

class General : public QWidget, public Ui::General
{
public:
  General( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );
  }
};


class ArkWidget : public KVBox
{
    Q_OBJECT
public:
    ArkWidget( QWidget *parent = 0 );
    virtual ~ArkWidget();

    bool isArchiveOpen() const { return m_bIsArchiveOpen; }
    int getNumFilesInArchive() const { return m_nNumFiles; }

    int getArkInstanceId() const { return m_arkInstanceId; }
    void setArkInstanceId( int aid ) { m_arkInstanceId = aid; }

    void cleanArkTmpDir();
    virtual QString getArchName() const { return m_strArchName; }

    const KUrl& realURL() const { return m_realURL; }
    void setRealURL( const KUrl& url ) { m_realURL = url; }

    QString tmpDir() const { return m_tmpDir ? m_tmpDir->name() : QString(); }

    FileListView * fileList() const { return m_fileListView; }
    K3ListViewSearchLine    * searchBar() const { return m_searchBar; }
    Arch * archive() const { return arch; }
    ArchType archiveType() const { return m_archType; }
    int numSelectedFiles() const { return m_nNumSelectedFiles; }

    /**
     * Miscellaneous tasks involved in closing an archive.
     */
    void closeArch();

    virtual void setExtractOnly(bool extOnly) { m_extractOnly = extOnly; }
    virtual void deleteAfterUse( const QString& path );
    bool allowedArchiveName( const KUrl & u );
    bool file_save_as( const KUrl & u );
    virtual KUrl getSaveAsFileName();
    virtual void setOpenAsMimeType( const QString & mimeType );
    QString & openAsMimeType(){ return m_openAsMimeType; }
    void prepareViewFiles( const QStringList & fileList );
    virtual void setArchivePopupEnabled( bool b );

    virtual void extractTo( const KUrl & targetDirectory, const KUrl & archive, bool bGuessName );
    virtual bool addToArchive( const KUrl::List & filesToAdd, const KUrl & archive = KUrl() );
    void convertTo( const KUrl & u );

    bool isModified() { return m_modified; }
    void setModified( bool b ) { m_modified = b; }

public slots:
    void file_open( const KUrl& url);
    virtual void file_close();
    virtual void file_new();
    void slotShowSearchBarToggled( bool b );
    void showSettings();

protected slots:
    void action_add();
    void action_add_dir();
    void action_view();
    void action_delete();
    bool action_extract();
    void slotOpenWith();
    void action_edit();

    void doPopup(Q3ListViewItem *, const QPoint &, int); // right-click menus
    void viewFile(Q3ListViewItem*); // doubleClick view files

    void slotSelectionChanged();
    void slotOpen(Arch *, bool, const QString &, int);
    void slotCreate(Arch *, bool, const QString &, int);
    void slotDeleteDone(bool);
    void slotExtractDone();
    void slotExtractRemoteDone(KJob *job);
    void slotAddDone(bool);
    void slotEditFinished(K3Process *);
signals:
    void openUrlRequest( const KUrl & url );
    void request_file_quit();
    void setBusy( const QString & );
    void setReady();
    void fixActions();
    void disableAllActions();
    void signalFilePopup( const QPoint & pPoint );
    void signalArchivePopup( const QPoint & pPoint );
    void setStatusBarText( const QString & text );
    void setStatusBarSelectedFiles( const QString & text );
    void removeRecentURL( const KUrl & url );
    void addRecentURL( const KUrl & url );
    void setWindowCaption( const QString &caption );
    void removeOpenArk( const KUrl & );
    void addOpenArk( const KUrl & );
    void createDone( bool );
    void openDone( bool );
    void createRealArchiveDone( bool );
    void extractRemoteMovingDone();

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
    void removeDownloadedFiles();

    // make sure that str is a local file/dir
    KUrl toLocalFile(const KUrl& url);

    // ask user whether to create a real archive from a compressed file
    // returns filename if so. Otherwise, empty.
    KUrl askToCreateRealArchive();
    Arch * getNewArchive( const QString & _fileName, const QString& _mimetype = QString() );
    void createRealArchive( const QString &strFilename,
                            const QStringList & filesToAdd = QStringList() );
    KUrl getCreateFilename( const QString & _caption,
                            const QString & _defaultMimeType = QString(),
                            bool allowCompressed = true,
                            const QString & _suggestedName = QString() );

    bool reportExtractFailures(const QString & _dest, QStringList *_list);
    QStringList existingFiles( const QString & _dest, QStringList & _list );

    void extractOnlyOpenDone();
    void extractRemoteInitiateMoving( const KUrl & target );
    void editStart();

    void busy( const QString & text );
    void holdBusy();
    void resumeBusy();
    void ready();

    //suggests an extract directory based on archive name
    const QString guessName( const KUrl & archive );

private slots:
    void startDrag( const QStringList & fileList );
    void startDragSlotExtractDone( bool );
    void editSlotExtractDone();
    void editSlotAddDone( bool success );
    void viewSlotExtractDone( bool success );
    void openWithSlotExtractDone();

    void createRealArchiveSlotCreate( Arch * newArch, bool success,
                                      const QString & fileName, int nbr );
    void createRealArchiveSlotAddDone( bool success );
    void createRealArchiveSlotAddFilesDone( bool success );

    void convertSlotExtractDone( bool success );
    void convertSlotCreate();
    void convertSlotCreateDone( bool success );
    void convertSlotAddDone( bool success );
    void convertFinish();

    void extractToSlotOpenDone( bool success );
    void extractToSlotExtractDone( bool success );

    void addToArchiveSlotOpenDone( bool success );
    void addToArchiveSlotCreateDone( bool success );
    void addToArchiveSlotAddDone( bool success );

protected:
    void arkWarning(const QString& msg);
    void arkError(const QString& msg);

    void newCaption(const QString& filename);
    void createFileListView();

    bool createArchive(const QString & name);
    void openArchive(const QString & name);

    void showCurrentFile();

private: // data

    bool m_bBusy;
    bool m_bBusyHold;

    // for use in the edit methods: the url.
    QString m_strFileToView;

    // the compressed file to be added into the new archive
    QString m_compressedFile;

    // Set to true if we are doing an "Extract to Folder"
    bool m_extractOnly;

    // Set to true if we are extracting to a remote location
    bool m_extractRemote;

    // URL to extract to.
    KUrl m_extractURL;

    // the mimetype the user wants to open this archive as
    QString m_openAsMimeType;

    // if they're dragging in files, this is the temporary list for when
    // we have to create an archive:
    QStringList *m_pTempAddList;

    KRun *m_pKRunPtr;

    QStringList mpDownloadedList;

    bool m_bArchivePopupEnabled;

    KTempDir * m_convert_tmpDir;
    KUrl m_convert_saveAsURL;
    bool m_convertSuccess;

    KUrl m_extractTo_targetDirectory;

    KUrl::List m_addToArchive_filesToAdd;
    KUrl m_addToArchive_archive;

    KTempDir * m_createRealArchTmpDir;
    KTempDir * m_extractRemoteTmpDir;

    bool m_modified;

    KToolBar  * m_searchToolBar;
    K3ListViewSearchLine * m_searchBar;

    Arch   * arch;
    QString  m_strArchName;
    KUrl     m_realURL;
    KUrl     m_url;
    ArchType m_archType;
    FileListView * m_fileListView;

    KIO::filesize_t m_nSizeOfFiles;
    KIO::filesize_t m_nSizeOfSelectedFiles;
    unsigned int m_nNumFiles;
    int m_nNumSelectedFiles;
    int m_arkInstanceId;

    bool m_bIsArchiveOpen;
    bool m_bIsSimpleCompressedFile;
    bool m_bDropSourceIsSelf;

    QStringList mDragFiles;
    QStringList *m_extractList;
    KTempDir *m_tmpDir;
};

#endif /* ARKWIDGET_H*/

