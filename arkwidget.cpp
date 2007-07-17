/*

 ark -- archiver for the KDE project

 Copyright (C) 2004-2005 Henrique Pinto <henrique.pinto@kdemail.net>
 Copyright (C) 2003 Georg Robbers <Georg.Robbers@urz.uni-hd.de>
 Copyright (C) 2002-2003 Helio Chissini de Castro <helio@conectiva.com.br>
 Copyright (C) 2001-2002 Roberto Teixeira <maragato@kde.org>
 Copyright (C) 2001 Corel Corporation (author: Michael Jarrett <michaelj@corel.com>)
 Copyright (C) 1999-2000 Corel Corporation (author: Emily Ezust <emilye@corel.com>)
 Copyright (C) 1999 Francois-Xavier Duranceau <duranceau@kde.org>
 Copyright (C) 1997-1999 Rob Palmbos <palm9744@kettering.edu>

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

// ark includes
#include "arkwidget.h"
#include "arkapp.h"
#include "archiveformatdlg.h"
#include "extractiondialog.h"
#include "filelistview.h"
#include "arkutils.h"
#include "archiveformatinfo.h"
#include "arkviewer.h"

#include <sys/types.h>
#include <sys/stat.h>

// Qt includes
#include <QLayout>
#include <qstringlist.h>
#include <QLabel>
#include <QCheckBox>
#include <QDir>
//Added by qt3to4:
#include <QDragMoveEvent>
#include <QDropEvent>

// KDE includes
#include <KDebug>
#include <KLocale>
#include <KMessageBox>
#include <KOpenWithDialog>
#include <KMimeType>
#include <KStandardDirs>
#include <KTempDir>
#include <KFileDialog>
#include <KDirSelectDialog>
#include <KTreeWidgetSearchLine>
#include <KToolBar>
#include <KConfigDialog>
#include <KUrl>
#include <KServiceTypeTrader>
#include <KRun>
#include <KIO/JobUiDelegate>
#include <KIO/NetAccess>
#include <KIO/CopyJob>

// settings
#include "settings.h"
#include <KMenu>
#include <KDialog>


static void viewInExternalViewer( ArkWidget* parent, const QString& filename )
{
    QString mimetype = KMimeType::findByUrl( filename )->name();
    bool view = true;

    if ( KRun::isExecutable( mimetype ) )
    {
        QString text = i18n( "The file you are trying to view may be an executable. Running untrusted executables may compromise your system's security.\nAre you sure you want to run that file?" );
        view = ( KMessageBox::warningContinueCancel( parent, text, QString(), KGuiItem(i18n("Run Nevertheless")) ) == KMessageBox::Continue );
    }

    if ( view )
        KRun::runUrl( filename, mimetype, 0L );
}

ArkWidget::ArkWidget( QWidget *parent )
   : KVBox(parent), m_bBusy( false ), m_bBusyHold( false ),
     m_extractOnly( false ), m_extractRemote(false),
     m_openAsMimeType( QString() ), m_pTempAddList(NULL),
     m_bArchivePopupEnabled( false ),
     m_extractRemoteTmpDir( NULL ),
     m_modified( false ), m_searchToolBar( 0 ), m_searchBar( 0 ),
     arch( 0 ), m_archType( UNKNOWN_FORMAT ), m_fileListView( 0 ),
     m_nSizeOfFiles( 0 ), m_nSizeOfSelectedFiles( 0 ), m_nNumFiles( 0 ),
     m_nNumSelectedFiles( 0 ), m_bIsArchiveOpen( false ),
     m_bDropSourceIsSelf( false ), m_extractList( 0 )
{
    m_tmpDir = new KTempDir( KStandardDirs::locateLocal( "tmp", "ark" ) );
    m_tmpDir->setAutoRemove(false);

    if ( m_tmpDir->status() != 0 )
    {
       kWarning( 1601 ) << "Could not create a temporary directory. status() returned "
                         << m_tmpDir->status() << "." << endl;
       m_tmpDir = NULL;
    }

    QSizePolicy searchBarSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    searchBarSizePolicy.setVerticalStretch(0);
    m_searchToolBar = new KToolBar( this, "searchBar");
    m_searchToolBar->setSizePolicy(searchBarSizePolicy);

    QLabel * l1 = new QLabel( i18n( "&Search:" ), m_searchToolBar );
    l1->setObjectName( "kde toolbar widget" );
    m_searchToolBar->addWidget(l1);

    m_searchBar = new KTreeWidgetSearchLine( m_searchToolBar , 0 );
    m_searchToolBar->addWidget(m_searchBar);

    l1->setBuddy( m_searchBar );

    if ( !ArkSettings::showSearchBar() )
        m_searchToolBar->hide();

    createFileListView();

    m_searchBar->addTreeWidget( m_fileListView );

    // enable DnD
    setAcceptDrops(true);
    setFocusProxy(m_searchBar);
}

ArkWidget::~ArkWidget()
{
    cleanArkTmpDir();
    ready();
    delete m_pTempAddList;
    delete m_fileListView;
    m_fileListView = 0;
    delete arch;
    ArkSettings::self()->writeConfig();
}

void ArkWidget::cleanArkTmpDir()
{
        removeDownloadedFiles();
   if ( m_tmpDir )
   {
      m_tmpDir->unlink();
      delete m_tmpDir;
      m_tmpDir = NULL;
   }
}

void ArkWidget::closeArch()
{
   if ( isArchiveOpen() )
   {
      delete arch;
      arch = 0;
      m_bIsArchiveOpen = false;
   }

   if ( m_fileListView )
   {
      m_fileListView->clear();
   }
}

void ArkWidget::updateStatusTotals()
{
    m_nNumFiles    = m_fileListView->totalFiles();
    m_nSizeOfFiles = m_fileListView->totalSize();

    QString strInfo = i18np( "%1 file  %2", "%1 files  %2", m_nNumFiles ,
                            KIO::convertSize( m_nSizeOfFiles ) );
    emit setStatusBarText(strInfo);
}

void ArkWidget::busy( const QString & text )
{
    emit setBusy( text );

    if ( m_bBusy )
        return;

    QApplication::setOverrideCursor( Qt::WaitCursor );
    m_bBusy = true;
}

void ArkWidget::holdBusy()
{
    if ( !m_bBusy || m_bBusyHold )
        return;

    m_bBusyHold = true;
    QApplication::restoreOverrideCursor();
}

void ArkWidget::resumeBusy()
{
    if ( !m_bBusyHold )
        return;

    m_bBusyHold = false;
    QApplication::setOverrideCursor( Qt::WaitCursor );
}

void ArkWidget::ready()
{
    if ( !m_bBusy )
        return;

    QApplication::restoreOverrideCursor();
    emit setReady();
    m_bBusyHold = false;
    m_bBusy = false;
}

KUrl ArkWidget::getSaveAsFileName()
{
    QString defaultMimeType;
    if ( m_openAsMimeType.isNull() )
        defaultMimeType = KMimeType::findByPath( m_strArchName )->name();
    else
        defaultMimeType = m_openAsMimeType;

    KUrl u;
    QString suggestedName;
    if ( m_realURL.isLocalFile() )
        suggestedName = m_realURL.url();
    else
        suggestedName = m_realURL.fileName( false );

    do
    {
        u = getCreateFilename( i18n( "Save Archive As" ), defaultMimeType, true, suggestedName );
        if (  u.isEmpty() )
            return u;
        if( allowedArchiveName( u ) || ( ArchiveFormatInfo::self()->archTypeByExtension( u.path() ) != UNKNOWN_FORMAT ) )
            break;
        KMessageBox::error( this, i18n( "Please save your archive in the same format as the original.\nHint: Use one of the suggested extensions." ) );
    }
    while ( true );
    return u;
}

bool ArkWidget::allowedArchiveName( const KUrl & u )
{
    if (u.isEmpty())
        return false;

    QString archMimeType = KMimeType::findByUrl( m_url )->name();
    if ( !m_openAsMimeType.isNull() )
        archMimeType = m_openAsMimeType;
    QString newArchMimeType = KMimeType::findByPath( u.path() )->name();
    if ( archMimeType == newArchMimeType )
        return true;

    return false;
}


const QString ArkWidget::guessName( const KUrl &archive )
{
  QString fileName = archive.fileName();
  QStringList list = KMimeType::findByPath( fileName )->patterns();
  QStringList::Iterator it = list.begin();
  QString ext;
  for ( ; it != list.end(); ++it )
  {
    ext = (*it).remove( '*' );
    if ( fileName.endsWith( ext ) )
    {
      fileName = fileName.left( fileName.lastIndexOf( ext ) );
      break;
    }
  }

  return fileName;
}

void ArkWidget::setOpenAsMimeType( const QString & mimeType )
{
    m_openAsMimeType = mimeType;
}

void ArkWidget::file_open(const KUrl& url)
{
    QString strFile = url.path();

    kDebug( 1601 ) << "File to open: " << strFile << endl;

    Q_ASSERT( QFileInfo( strFile ).exists() );

    if ( !QFileInfo( strFile ).isReadable() )
    {
        KMessageBox::error(this, i18n("You do not have permission to access that archive.") );
        emit removeRecentURL( m_realURL );
        return;
    }

    // Set the current archive filename to the filename
    m_strArchName = strFile;
    m_url = url;

    openArchive( strFile );
}


KUrl ArkWidget::getCreateFilename(const QString & _caption,
                                  const QString & _defaultMimeType,
                                  bool allowCompressed,
                                  const QString & _suggestedName )
{
    int choice=0;
    bool fileExists = true;
    QString strFile;
    KUrl url;

    KFileDialog dlg( KUrl("kfiledialog://ArkSaveAsDialog"), QString::null, this );
    dlg.setCaption( _caption );
    dlg.setOperationMode( KFileDialog::Saving );
    dlg.setMimeFilter( Arch::supportedMimeTypes(),
                       _defaultMimeType.isNull() ?  "application/x-compressed-tar" : _defaultMimeType );
    if ( !_suggestedName.isEmpty() )
        dlg.setSelection( _suggestedName );

    while ( fileExists )
        // keep asking for filenames as long as the user doesn't want to
        // overwrite existing ones; break if they agree to overwrite
        // or if the file doesn't already exist. Return if they cancel.
        // Also check for proper extensions.
    {
        dlg.exec();
        url = dlg.selectedUrl();
        strFile = url.path();

        if (strFile.isEmpty())
            return KUrl();

        //the user chose to save as the current archive
        //or wanted to create a new one with the same name
        //no need to do anything
        if (strFile == m_strArchName && m_bIsArchiveOpen)
            return KUrl();

        QStringList extensions = dlg.currentFilterMimeType()->patterns();
        QStringList::Iterator it = extensions.begin();
        for ( ; it != extensions.end() && !strFile.endsWith( ( *it ).remove( '*' ) ); ++it )
            ;

        if ( it == extensions.end() )
        {
            strFile += ArchiveFormatInfo::self()->defaultExtension( dlg.currentFilterMimeType()->name() );
            url.setPath( strFile );
        }

        kDebug(1601) << "Trying to create an archive named " << strFile << endl;
        fileExists = QFile::exists( strFile );
        if( fileExists )
        {
            choice = KMessageBox::warningYesNoCancel(0,
               i18n("Archive already exists. Do you wish to overwrite it?"),
               i18n("Archive Already Exists"), KStandardGuiItem::overwrite(), KGuiItem(i18n("Do Not Overwrite")));

            if ( choice == KMessageBox::Yes )
            {
                QFile::remove( strFile );
                break;
            }
            else if ( choice == KMessageBox::Cancel )
            {
                return KUrl();
            }
            else
            {
                continue;
            }
        }
        // if we got here, the file does not already exist.
        if ( !ArkUtils::haveDirPermissions( url.directory() ) )
        {
            KMessageBox::error( this,
                i18n( "You do not have permission"
                      " to write to the directory %1" , url.directory() ) );
            return KUrl();
        }
    } // end of while loop

    return url;
}

void ArkWidget::file_new()
{
    QString strFile;
    KUrl url = getCreateFilename(i18n("Create New Archive") );
    strFile = url.path();
    if (!strFile.isEmpty())
    {
        file_close();
        createArchive( strFile );
    }
}

void
ArkWidget::extractOnlyOpenDone()
{
    bool done = action_extract();

    // last extract dir is still set, but this is not a problem
    if( !done )
    {
        emit request_file_quit();
    }

}

void
ArkWidget::slotExtractDone()
{
    disconnect( arch, SIGNAL( sigExtract( bool ) ),
                this, SLOT( slotExtractDone() ) );
    ready();

    if(m_extractList != 0)
        delete m_extractList;
    m_extractList = 0;

    if( m_fileListView ) // avoid race condition, don't do updates if application is exiting
    {
        m_fileListView->setUpdatesEnabled(true);
        fixEnables();
    }

    if ( m_extractRemote )
    {
        extractRemoteInitiateMoving( m_extractURL );
    }
    else if( m_extractOnly )
    {
        emit request_file_quit();
    }

    kDebug(1601) << "-ArkWidget::slotExtractDone" << endl;
}

void
ArkWidget::extractRemoteInitiateMoving( const KUrl & target )
{
    KUrl srcDirURL;
    KUrl src;
    QString srcDir;

    srcDir = m_extractRemoteTmpDir->name();
    srcDirURL.setPath( srcDir );

    QDir dir( srcDir );
    dir.setFilter( QDir::TypeMask | QDir::Hidden );
    QStringList lst( dir.entryList() );
    lst.removeAll( "." );
    lst.removeAll( ".." );

    KUrl::List srcList;
    for( QStringList::ConstIterator it = lst.begin(); it != lst.end() ; ++it)
    {
        src = srcDirURL;
        src.addPath( *it );
        srcList.append( src );
    }

    m_extractURL.adjustPath( KUrl::AddTrailingSlash );

    KIO::CopyJob *job = KIO::copy( srcList, target, this );
    job->ui()->setWindow( this );
    connect( job, SIGNAL(result(KJob*)),
            this, SLOT(slotExtractRemoteDone(KJob*)) );

    m_extractRemote = false;
}

void
ArkWidget::slotExtractRemoteDone(KJob *job)
{
    delete m_extractRemoteTmpDir;
    m_extractRemoteTmpDir = NULL;

    if ( job->error() )
        static_cast<KIO::Job*>( job )->ui()->showErrorMessage();

    emit extractRemoteMovingDone();

    if ( m_extractOnly )
        emit request_file_quit();
}


void
ArkWidget::disableAll() // private
{
    emit disableAllActions();
    m_fileListView->setUpdatesEnabled(true);
}

void
ArkWidget::fixEnables() // private
{
    emit fixActions(); //connected to the part
}

void
ArkWidget::file_close()
{
    if ( isArchiveOpen() )
    {
        closeArch();
        emit setWindowCaption( QString() );
        emit removeOpenArk( m_strArchName );
        updateStatusTotals();
        updateStatusSelection();
        fixEnables();
    }
    else
    {
        closeArch();
    }

    m_strArchName.clear();
    m_url = KUrl();
}

// Action menu /////////////////////////////////////////////////////////

void
ArkWidget::action_add()
{
    KFileDialog fileDlg( KUrl("kfiledialog://ArkAddDir"), QString::null, this );
    fileDlg.setMode( KFile::Files | KFile::ExistingOnly );
    fileDlg.setCaption(i18n("Select Files to Add"));

    if(fileDlg.exec())
    {
        KUrl::List addList;
        addList = fileDlg.selectedUrls();
        QStringList list;
        foreach( const KUrl &url, addList )
            list << KUrl::fromPercentEncoding( url.url().toLatin1() );

        if ( list.count() > 0 )
        {
            addFile( list );
        }
    }
}

void
ArkWidget::addFile( const QStringList &list )
{
    if ( !ArkUtils::diskHasSpace( tmpDir(), ArkUtils::getSizes( list ) ) )
        return;

    disableAll();
    busy( i18n( "Adding files..." ) );
    // if they are URLs, we have to download them, replace the URLs
    // with filenames, and remember to delete the temporaries later.
    QStringList localList;
    foreach( const QString &str, list )
    {
        localList << toLocalFile( KUrl( str ) ).prettyUrl();
    }

    connect( arch, SIGNAL( sigAdd( bool ) ), this, SLOT( slotAddDone( bool ) ) );
    arch->addFile( list );
}

void
ArkWidget::action_add_dir()
{
    KUrl u = KDirSelectDialog::selectDirectory( KUrl("kfiledialog://ArkAddDir"),
                                                false, this,
                                                i18n("Select Folder to Add"));

    QString dir = KUrl::fromPercentEncoding( u.url( KUrl::RemoveTrailingSlash ).toLatin1() );
    if ( !dir.isEmpty() )
    {
        busy( i18n( "Adding folder..." ) );
        disableAll();
        u = toLocalFile(u);
        connect( arch, SIGNAL( sigAdd( bool ) ), this, SLOT( slotAddDone( bool ) ) );
        arch->addDir( u.prettyUrl() );
    }

}

void
ArkWidget::slotAddDone(bool _bSuccess)
{
    disconnect( arch, SIGNAL( sigAdd( bool ) ), this, SLOT( slotAddDone( bool ) ) );
    m_fileListView->setUpdatesEnabled(true);
    ready();

    if (_bSuccess)
    {
        m_modified = true;
        //simulate reload
        KUrl u;
        u.setPath( arch->fileName() );
        file_close();
        file_open( u );
        emit setWindowCaption( u.path() );
    }
    removeDownloadedFiles();
    fixEnables();
}



KUrl
ArkWidget::toLocalFile( const KUrl& url )
{
    KUrl localURL = url;

    if(!url.isLocalFile())
    {
   QString strURL = url.prettyUrl();

        QString tempfile = tmpDir();
        tempfile += strURL.right(strURL.length() - strURL.lastIndexOf('/') - 1);
        deleteAfterUse(tempfile);  // remember for deletion
        KUrl tempurl; tempurl.setPath( tempfile );
        if( !KIO::NetAccess::dircopy(url, tempurl, this) )
            return KUrl();
        localURL = tempfile;
    }
    return localURL;
}

void
ArkWidget::deleteAfterUse( const QString& path )
{
    mpDownloadedList.append( path );
}

void
ArkWidget::removeDownloadedFiles()
{
    if (!mpDownloadedList.isEmpty())
    {
        // It is necessary to remove those files even if tmpDir() is getting deleted:
        // not all files in mpDownloadedList are from tmpDir() - e.g. when using --tempfile
        // But of course we could decide to not add files from tmpDir() into mpDownloadedList.
        QStringList::ConstIterator it = mpDownloadedList.begin();
        QStringList::ConstIterator end = mpDownloadedList.end();
        for ( ; it != end ; ++it )
            QFile::remove( *it );
        mpDownloadedList.clear();
    }
}

void
ArkWidget::action_delete()
{
    // remove selected files and create a list to send to the archive
    // Warn the user if he/she/it tries to delete a directory entry in
    // a tar file - it actually deletes the contents of the directory
    // as well.

    if (m_fileListView->isSelectionEmpty())
    {
        return; // shouldn't happen - delete should have been disabled!
    }

    QStringList list = m_fileListView->selectedFilenames();

    // ask for confirmation
    if ( !KMessageBox::warningContinueCancel( this,
                                              i18n( "Do you really want to delete the selected items?" ),
                                              QString(),
                                              KStandardGuiItem::del() )
         == KMessageBox::Continue)
    {
        return;
    }

    // Remove the entries from the list view
    QTreeWidgetItemIterator it( m_fileListView, QTreeWidgetItemIterator::Selected );
    while ( *it )
    {
        delete *it;
        ++it;
    }

    disableAll();
    busy( i18n( "Removing..." ) );
    connect( arch, SIGNAL( sigDelete( bool ) ), this, SLOT( slotDeleteDone( bool ) ) );
    arch->remove(list);
    kDebug(1601) << "-ArkWidget::action_delete" << endl;
}

void
ArkWidget::slotDeleteDone(bool _bSuccess)
{
    disconnect( arch, SIGNAL( sigDelete( bool ) ), this, SLOT( slotDeleteDone( bool ) ) );
    kDebug(1601) << "+ArkWidget::slotDeleteDone" << endl;
    m_fileListView->setUpdatesEnabled(true);
    if (_bSuccess)
    {
        m_modified = true;
        updateStatusTotals();
        updateStatusSelection();
    }
    // disable the select all and extract options if there are no files left
    fixEnables();
    ready();
    kDebug(1601) << "-ArkWidget::slotDeleteDone" << endl;

}



void
ArkWidget::slotOpenWith()
{
    connect( arch, SIGNAL( sigExtract( bool ) ), this,
            SLOT( openWithSlotExtractDone() ) );

    showCurrentFile();
}

void
ArkWidget::openWithSlotExtractDone()
{
    disconnect( arch, SIGNAL( sigExtract( bool ) ), this,
            SLOT( openWithSlotExtractDone() ) );

    KUrl::List list;
    KUrl url = m_strFileToView;
    list.append(url);
    KOpenWithDialog l( list, i18n("Open with:"), QString(), this);
    if ( l.exec() )
    {
        KService::Ptr service = l.service();
        if ( !!service )
        {
            KRun::run( *service, list,this );
        }
        else
        {
            QString exec = l.text();
            exec += " %f";
            KRun::run( exec, list, topLevelWidget());
        }
    }
    if( m_fileListView )
    {
        m_fileListView->setUpdatesEnabled(true);
        fixEnables();
    }

}


void
ArkWidget::prepareViewFiles( const QStringList & fileList )
{
    QString destTmpDirectory;
    destTmpDirectory = tmpDir();

    QList<QVariant> files;
    // Make sure to delete previous file already there...
    foreach( const QString &name, fileList )
    {
        QFile::remove( destTmpDirectory + name );
        files << name;
    }


    arch->extractFiles( files, destTmpDirectory );
}

bool
ArkWidget::reportExtractFailures( const QString & _dest, QStringList *_list )
{
    // reports extract failures when Overwrite = False and the file
    // exists already in the destination directory.
    // If list is null, it means we are extracting all files.
    // Otherwise the list contains the files we are to extract.

    bool redoExtraction = false;
    QString strFilename;

    QStringList list = *_list;
    QStringList filesExisting = existingFiles( _dest, list );

    int numFilesToReport = filesExisting.count();

    // now report on the contents
    holdBusy();
    if (numFilesToReport == 1)
    {
        kDebug(1601) << "One to report" << endl;
        strFilename = filesExisting.first();
        QString message = i18n("%1 will not be extracted because it will overwrite an existing file.\nGo back to the Extraction Dialog?", strFilename);
        redoExtraction = (KMessageBox::questionYesNo(this, message) == KMessageBox::Yes);
    }
    else if (numFilesToReport != 0)
    {
        QString message = i18n("Some files will not be extracted, because they would overwrite existing files.\nWould you like to go back to the extraction dialog?\n\nThe following files will not be extracted if you choose to continue:");
        redoExtraction = (KMessageBox::questionYesNoList(this, message, filesExisting) == KMessageBox::Yes);
    }
    resumeBusy();
    return redoExtraction;
}

QStringList
ArkWidget::existingFiles( const QString & _dest, QStringList & _list )
{
    QString strFilename, tmp;

    QString strDestDir = _dest;

    // make sure the destination directory has a / at the end.
    if ( !strDestDir.endsWith( "/" ) )
    {
        strDestDir += '/';
    }

    if (_list.isEmpty())
    {
        _list = m_fileListView->fileNames();
    }

    QStringList existingFiles;
    // now the list contains all the names we must verify.
    for (QStringList::Iterator it = _list.begin(); it != _list.end(); ++it)
    {
        strFilename = *it;
        QString strFullName = strDestDir + strFilename;
        if ( QFile::exists( strFullName ) )
        {
            existingFiles.append( strFilename );
        }
    }
    return existingFiles;
}





bool
ArkWidget::action_extract()
{
    KUrl fileToExtract;
    fileToExtract.setPath( arch->fileName() );

     //before we start, make sure the archive is still there
    if (!KIO::NetAccess::exists( fileToExtract.prettyUrl(), true, this ) )
    {
        KMessageBox::error(0, i18n("The archive to extract from no longer exists."));
        return false;
    }

    //if more than one entry in the archive is root level, suggest a path prefix
    QString prefix = m_fileListView->topLevelItemCount() > 1 ?
                           QChar( '/' ) + guessName( realURL() )
                         : QString();

    // Should the extraction dialog show an option for extracting only selected files?
    bool enableSelected = ( m_nNumSelectedFiles > 0 );

    QString base = ArkSettings::extractionHistory().isEmpty()?
                       QString() : ArkSettings::extractionHistory().first();
    if ( base.isEmpty() )
    {
        // Perhaps the KDE Documents folder is a better choice?
        base = QDir::homePath();
    }

    // Default URL shown in the extraction dialog;
    KUrl defaultDir( base );

    if ( m_extractOnly )
    {
        defaultDir = KUrl::fromPath( QDir::currentPath() );
    }

    ExtractionDialog *dlg = new ExtractionDialog( this, enableSelected, defaultDir, prefix,  m_url.fileName() );

    bool bRedoExtract = false;

    // list of files to be extracted
    m_extractList = new QStringList;
    if ( dlg->exec() )
    {
        //m_extractURL will always be the location the user chose to
        //m_extract to, whether local or remote
        m_extractURL = KIO::NetAccess::mostLocalUrl( dlg->extractionDirectory(), this );

        //extractDir will either be the real, local extract dir,
        //or in case of a extract to remote location, a local tmp dir
        QString extractDir;

        if ( !m_extractURL.isLocalFile() )
        {
            m_extractRemoteTmpDir = new KTempDir( tmpDir() + "extremote" );

            extractDir = m_extractRemoteTmpDir->name();
            m_extractRemote = true;
            if ( m_extractRemoteTmpDir->status() != 0 )
            {
                kWarning( 1601 ) << "Unable to create temporary directory" << extractDir << endl;
                m_extractRemote = false;
                delete dlg;
                return false;
            }
        }
        else
        {
            extractDir = m_extractURL.path();
        }

        // if overwrite is false, then we need to check for failure of
        // extractions.
        bool bOvwrt = ArkSettings::extractOverwrite();

        if ( dlg->selectedOnly() == false )
        {
            if (!bOvwrt)  // send empty list to indicate we're extracting all
            {
                bRedoExtract = reportExtractFailures(extractDir, m_extractList);
            }

            if (!bRedoExtract) // if the user's OK with those failures, go ahead
            {
                // unless we have no space!
                if ( ArkUtils::diskHasSpace( extractDir, m_nSizeOfFiles ) )
                {
                    disableAll();
                    busy( i18n( "Extracting..." ) );
                    connect( arch, SIGNAL( sigExtract( bool ) ), this, SLOT( slotExtractDone() ) );
                    arch->extractFiles(QList<QVariant>(), extractDir);
                }
            }
        }
        else
        {
                KIO::filesize_t nTotalSize = 0;
                QStringList selectedFiles = m_fileListView->selectedFilenames();
                QList<QVariant> files;
                foreach( const QString& file, selectedFiles )
                {
                    m_extractList->append( QFile::encodeName( file ) );
                    files << file;
                }

                if (!bOvwrt)
                {
                    bRedoExtract = reportExtractFailures(extractDir, m_extractList);
                }
                if (!bRedoExtract)
                {
                    if (ArkUtils::diskHasSpace(extractDir, nTotalSize))
                    {
                        disableAll();
                        busy( i18n( "Extracting..." ) );
                        connect( arch, SIGNAL( sigExtract( bool ) ),
                                 this, SLOT( slotExtractDone() ) );
                        arch->extractFiles(files, extractDir); // extract selected files
                    }
                }
        }
        if ( dlg->viewFolderAfterExtraction() )
        {
            KRun::runUrl( dlg->extractionDirectory(), "inode/directory",this );
        }

        delete dlg;
    }
    else
    {
        delete dlg;
        return false;
    }

    // user might want to change some options or the selection...
    if (bRedoExtract)
    {
        return action_extract();
    }

    return true;
}

void
ArkWidget::action_view()
{
    connect( arch, SIGNAL( sigExtract( bool ) ), this,
             SLOT( viewSlotExtractDone( bool ) ) );
    busy( i18n( "Extracting file to view" ) );
    showCurrentFile();
}

void
ArkWidget::viewSlotExtractDone( bool success )
{
    if ( success )
    {
        chmod( QFile::encodeName( m_strFileToView ), 0400 );
        bool view = true;

        if ( ArkSettings::useIntegratedViewer() )
        {
            ArkViewer * viewer = new ArkViewer( this );
            viewer->setObjectName( "ArkViewer" );

            if ( !viewer->view( m_strFileToView ) )
            {
                QString text = i18n( "The internal viewer is not able to display this file. Would you like to view it using an external program?" );
                view = ( KMessageBox::warningYesNo( this, text, QString(), KGuiItem(i18n("View Externally")), KGuiItem(i18n("Do Not View")) ) == KMessageBox::Yes );

                if ( view )
                    viewInExternalViewer( this, m_strFileToView );
            }
        }
        else
        {
            viewInExternalViewer( this, m_strFileToView );
        }
    }

    disconnect( arch, SIGNAL( sigExtract( bool ) ), this,
                SLOT( viewSlotExtractDone( bool ) ) );
    // avoid race condition, don't do updates if application is exiting
    if( m_fileListView )
    {
        m_fileListView->setUpdatesEnabled(true);
        fixEnables();
    }
    ready();
}


void
ArkWidget::showCurrentFile()
{
    if ( !m_fileListView->currentItem() )
        return;

    ArchiveEntry entry = m_fileListView->currentEntry();
    QString name = entry[ FileName ].toString();

    QString fullname;
    fullname = "file:";
    fullname += tmpDir();
    fullname += name;

    kDebug(1601) << "File to be viewed: " << fullname << endl;

    QStringList extractList;
    extractList.append(name);

    m_strFileToView = fullname;
    if (ArkUtils::diskHasSpace( tmpDir(), entry[ Size ].toULongLong() ) )
    {
        disableAll();
        prepareViewFiles( extractList );
    }
}

// Popup /////////////////////////////////////////////////////////////

void
ArkWidget::setArchivePopupEnabled( bool b )
{
    m_bArchivePopupEnabled = b;
}

void
ArkWidget::doPopup( QTreeWidgetItem *pItem, const QPoint &pPoint, int nCol ) // slot
// do the right-click popup menus
{
    if ( nCol == 0 || !m_bArchivePopupEnabled )
    {
        m_fileListView->setCurrentItem(pItem);
        pItem->setSelected( true );
        emit signalFilePopup( pPoint );
    }
    else // clicked anywhere else but the name column
    {
        emit signalArchivePopup( pPoint );
    }
}


void
ArkWidget::viewFile( QTreeWidgetItem* item ) // slot
// show contents when double click
{
    // Preview, if it is a file
    if ( item->childCount() == 0)
        emit action_view();
    else  // Change opened state if it is a dir
        item->setExpanded( !item->isExpanded() );
}


// Service functions /////////////////////////////////////////////////

void
ArkWidget::slotSelectionChanged()
{
    updateStatusSelection();
}


////////////////////////////////////////////////////////////////////
//////////////////// updateStatusSelection /////////////////////////
////////////////////////////////////////////////////////////////////

void
ArkWidget::updateStatusSelection()
{
    m_nNumSelectedFiles    = m_fileListView->selectedFilesCount();
    m_nSizeOfSelectedFiles = m_fileListView->selectedSize();

    QString strInfo;
    if (m_nNumSelectedFiles == 0)
    {
        strInfo = i18n("0 files selected");
    }
    else
    {
        strInfo = i18ncp("%2 is the total size of selected files",
                         "1 files selected  %2", "%1 files selected  %2",
                         m_nNumSelectedFiles,
                         KIO::convertSize(m_nSizeOfSelectedFiles));
    }

    emit setStatusBarSelectedFiles(strInfo);
    fixEnables();
}


void
ArkWidget::arkWarning(const QString& msg)
{
    KMessageBox::information(this, msg);
}

void
ArkWidget::createFileListView()
{
   kDebug(1601) << "ArkWidget::createFileListView" << endl;
   if ( !m_fileListView )
   {
      m_fileListView = new FileListView(this);

      connect( m_fileListView, SIGNAL( itemSelectionChanged() ),
               this, SLOT( slotSelectionChanged() ) );
      connect( m_fileListView, SIGNAL( rightButtonPressed(QTreeWidgetItem *, const QPoint &, int) ),
            this, SLOT(doPopup(QTreeWidgetItem *, const QPoint &, int)));
      connect( m_fileListView, SIGNAL( itemActivated( QTreeWidgetItem *, int ) ),
            this, SLOT( viewFile(QTreeWidgetItem*) ) );
    }
    m_fileListView->clear();
}


Arch * ArkWidget::getNewArchive( const QString & _fileName, const QString& _mimetype )
{
    Arch * newArch = 0;

    QString type = _mimetype.isNull()? KMimeType::findByUrl( KUrl::fromPath(_fileName) )->name() : _mimetype;
    ArchType archtype = ArchiveFormatInfo::self()->archTypeForMimeType(type);
    kDebug( 1601 ) << "archtype is recognised as: " << archtype << endl;
    if(0 == (newArch = Arch::archFactory(archtype, _fileName, _mimetype)))
    {
        KMessageBox::error(this, i18n("Unknown archive format or corrupted archive") );
        emit request_file_quit();
        return NULL;
    }

    m_archType = archtype;
    m_fileListView->setUpdatesEnabled(true);
    return newArch;
}

//////////////////////////////////////////////////////////////////////
////////////////////// createArchive /////////////////////////////////
//////////////////////////////////////////////////////////////////////


bool
ArkWidget::createArchive( const QString & _filename )
{
    Arch * newArch = getNewArchive( _filename );
    if ( !newArch )
        return false;

    busy( i18n( "Creating archive..." ) );
    connect( newArch, SIGNAL(sigCreate(Arch *, bool, const QString &, int) ),
             this, SLOT(slotCreate(Arch *, bool, const QString &, int) ) );

    newArch->create();
    return true;
}

void
ArkWidget::slotCreate(Arch * _newarch, bool _success, const QString & _filename, int)
{
    kDebug( 1601 ) << k_funcinfo << endl;
    disconnect( _newarch, SIGNAL( sigCreate( Arch *, bool, const QString &, int ) ),
                this, SLOT(slotCreate(Arch *, bool, const QString &, int) ) );
    ready();
    if ( _success )
    {
        //file_close(); already called in ArkWidget::file_new()
        m_strArchName = _filename;
        // for the hack in compressedfile; needed when creating a compressedfile
        // then directly adding a file
        KUrl u;
        u.setPath( _filename );
        setRealURL( u );

        emit setWindowCaption( _filename );
        emit addRecentURL( u );
        createFileListView();
        m_fileListView->show();
        m_bIsArchiveOpen = true;
        arch = _newarch;
        fixEnables();
    }
    else
    {
        KMessageBox::error(this, i18n("An error occurred while trying to create the archive.") );
    }
    emit createDone( _success );
}

//////////////////////////////////////////////////////////////////////
//////////////////////// openArchive /////////////////////////////////
//////////////////////////////////////////////////////////////////////

void
ArkWidget::openArchive( const QString & _filename )
{
    Arch *newArch = Arch::archFactory( UNKNOWN_FORMAT, _filename, m_openAsMimeType);
    if ( !newArch )
    {
        emit setWindowCaption( QString() );
        emit removeRecentURL( m_realURL );
        KMessageBox::error( this, i18n("Unknown archive format or corrupted archive") );
        return;
    }

    connect( newArch, SIGNAL( opened( bool ) ),
             this, SLOT( slotOpened( bool ) ) );
    connect( newArch, SIGNAL( newEntry( const ArchiveEntry& ) ),
             m_fileListView, SLOT( addItem( const ArchiveEntry& ) ) );

    disableAll();

    busy( i18n( "Opening the archive..." ) );
    m_fileListView->setUpdatesEnabled( false );
    arch = newArch;
    newArch->open();
    emit addRecentURL( m_url );
}

void
ArkWidget::slotOpened( bool success )
{
    kDebug( 1601 ) << k_funcinfo << " success = " << success << endl;
    ready();
    m_fileListView->setUpdatesEnabled(true);

    m_fileListView->show();

    if ( success )
    {
        if ( arch->isReadOnly() )
        {
            KMessageBox::information(this, i18n("This archive is read-only. If you want to save it under a new name, go to the File menu and select Save As."), i18n("Information"), "ReadOnlyArchive");
        }
        updateStatusTotals();
        m_bIsArchiveOpen = true;

        if ( m_extractOnly )
        {
            extractOnlyOpenDone();
            return;
        }
        emit addOpenArk( arch->fileName() );
    }
    else
    {
        emit removeRecentURL( m_realURL );
        emit setWindowCaption( QString() );
        KMessageBox::error( this, i18n( "An error occurred while trying to open the archive %1", arch->fileName() ) );

        if ( m_extractOnly )
            emit request_file_quit();
    }

    fixEnables();
    emit openDone( success );
}


void ArkWidget::slotShowSearchBarToggled( bool b )
{
   if ( b )
   {
      m_searchToolBar->show();
      ArkSettings::setShowSearchBar( true );
   }
   else
   {
      m_searchToolBar->hide();
      ArkSettings::setShowSearchBar( false );
   }
}

/**
 * Show Settings dialog.
 */
void ArkWidget::showSettings()
{
  if(KConfigDialog::showDialog("settings"))
    return;

  KConfigDialog *dialog = new KConfigDialog(this, "settings", ArkSettings::self());

  General* genPage = new General(0);
  dialog->addPage(genPage, i18n("General"), "ark", i18n("General Settings"));

  KService::List offers;

  offers = KServiceTypeTrader::self()->query( "KonqPopupMenu/Plugin", "Library == 'libarkplugin'" );

  if ( offers.isEmpty() )
  {
     genPage->kcfg_KonquerorIntegration->setEnabled( false );
  }
  else
  {
     genPage->konqIntegrationLabel->setText( QString() );
  }

  dialog->show();
}

#include "arkwidget.moc"
