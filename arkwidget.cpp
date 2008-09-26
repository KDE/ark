/*

 ark -- archiver for the KDE project

 Copyright (C)

 2004-2005: Henrique Pinto <henrique.pinto@kdemail.net>
 2003: Georg Robbers <Georg.Robbers@urz.uni-hd.de>
 2002-2003: Helio Chissini de Castro <helio@conectiva.com.br>
 2001-2002: Roberto Teixeira <maragato@kde.org>
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

#include <sys/types.h>
#include <sys/stat.h>

// Qt includes
#include <qlayout.h>
#include <qstringlist.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qdir.h>

// KDE includes
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kio/netaccess.h>
#include <kio/job.h>
#include <kopenwith.h>
#include <ktempfile.h>
#include <kmimemagic.h>
#include <kmimetype.h>
#include <kstandarddirs.h>
#include <ktempdir.h>
#include <kprocess.h>
#include <kfiledialog.h>
#include <kdirselectdialog.h>
#include <kurldrag.h>
#include <klistviewsearchline.h>
#include <ktoolbar.h>
#include <kconfigdialog.h>
#include <ktrader.h>
#include <kurl.h>

// settings
#include "settings.h"
#include "general.h"
#include "addition.h"
#include "extraction.h"
#include <kpopupmenu.h>
#include <kdialog.h>

// ark includes
#include "arkapp.h"
#include "archiveformatdlg.h"
#include "extractiondialog.h"
#include "arkwidget.h"
#include "filelistview.h"
#include "arkutils.h"
#include "archiveformatinfo.h"
#include "compressedfile.h"
#include "searchbar.h"
#include "arkviewer.h"

static void viewInExternalViewer( ArkWidget* parent, const KURL& filename )
{
    QString mimetype = KMimeType::findByURL( filename )->name();
    bool view = true;

    if ( KRun::isExecutable( mimetype ) )
    {
        QString text = i18n( "The file you're trying to view may be an executable. Running untrusted executables may compromise your system's security.\nAre you sure you want to run that file?" );
        view = ( KMessageBox::warningContinueCancel( parent, text, QString::null, i18n("Run Nevertheless") ) == KMessageBox::Continue );
    }

    if ( view )
        KRun::runURL( filename, mimetype );

}

//----------------------------------------------------------------------
//
//  Class ArkWidget starts here
//
//----------------------------------------------------------------------

ArkWidget::ArkWidget( QWidget *parent, const char *name )
   : QVBox(parent, name), m_bBusy( false ), m_bBusyHold( false ),
     m_extractOnly( false ), m_extractRemote(false),
     m_openAsMimeType(QString::null), m_pTempAddList(NULL),
     m_bArchivePopupEnabled( false ),
     m_convert_tmpDir( NULL ), m_convertSuccess( false ),
     m_createRealArchTmpDir( NULL ),  m_extractRemoteTmpDir( NULL ),
     m_modified( false ), m_searchToolBar( 0 ), m_searchBar( 0 ),
     arch( 0 ), m_archType( UNKNOWN_FORMAT ), m_fileListView( 0 ),
     m_nSizeOfFiles( 0 ), m_nSizeOfSelectedFiles( 0 ), m_nNumFiles( 0 ),
     m_nNumSelectedFiles( 0 ), m_bIsArchiveOpen( false ),
     m_bIsSimpleCompressedFile( false ),
     m_bDropSourceIsSelf( false ), m_extractList( 0 )
{
    m_tmpDir = new KTempDir( locateLocal( "tmp", "ark" ) );

    if ( m_tmpDir->status() != 0 )
    {
       kdWarning( 1601 ) << "Could not create a temporary directory. status() returned "
                         << m_tmpDir->status() << "." << endl;
       m_tmpDir = NULL;
    }

    m_searchToolBar = new KToolBar( this, "searchBar" );
    m_searchToolBar->boxLayout()->setSpacing( KDialog::spacingHint() );

    QLabel * l1 = new QLabel( i18n( "&Search:" ), m_searchToolBar, "kde toolbar widget" );
    m_searchBar = new SearchBar( m_searchToolBar, 0 );
    l1->setBuddy( m_searchBar );

    m_searchToolBar->setStretchableWidget( m_searchBar );

    if ( !ArkSettings::showSearchBar() )
        m_searchToolBar->hide();

    createFileListView();

    m_searchBar->setListView( m_fileListView );

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
    ArkSettings::writeConfig();
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
      m_fileListView->clearHeaders();
   }
}

////////////////////////////////////////////////////////////////////
///////////////////////// updateStatusTotals ///////////////////////
////////////////////////////////////////////////////////////////////

void
ArkWidget::updateStatusTotals()
{
    m_nNumFiles    = m_fileListView->totalFiles();
    m_nSizeOfFiles = m_fileListView->totalSize();

    QString strInfo = i18n( "%n file  %1", "%n files  %1", m_nNumFiles )
                          .arg( KIO::convertSize( m_nSizeOfFiles ) );
    emit setStatusBarText(strInfo);
}

void
ArkWidget::busy( const QString & text )
{
    emit setBusy( text );

    if ( m_bBusy )
        return;

    m_fileListView->setEnabled( false );

    QApplication::setOverrideCursor( waitCursor );
    m_bBusy = true;
}

void
ArkWidget::holdBusy()
{
    if ( !m_bBusy || m_bBusyHold )
        return;

    m_bBusyHold = true;
    QApplication::restoreOverrideCursor();
}

void
ArkWidget::resumeBusy()
{
    if ( !m_bBusyHold )
        return;

    m_bBusyHold = false;
    QApplication::setOverrideCursor( waitCursor );
}

void
ArkWidget::ready()
{
    if ( !m_bBusy )
        return;

    m_fileListView->setEnabled( true );

    QApplication::restoreOverrideCursor();
    emit setReady();
    m_bBusyHold = false;
    m_bBusy = false;
}

//////////////////////////////////////////////////////////////////////
////////////////////// file_save_as //////////////////////////////////
//////////////////////////////////////////////////////////////////////

KURL
ArkWidget::getSaveAsFileName()
{
    QString defaultMimeType;
    if ( m_openAsMimeType.isNull() )
        defaultMimeType = KMimeType::findByPath( m_strArchName )->name();
    else
        defaultMimeType = m_openAsMimeType;

    KURL u;
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

bool
ArkWidget::file_save_as( const KURL & u )
{
    bool success = KIO::NetAccess::upload( m_strArchName, u, this );
    if ( m_modified && success )
        m_modified = false;
    return success;
}

void
ArkWidget::convertTo( const KURL & u )
{
    busy( i18n( "Saving..." ) );
    m_convert_tmpDir =  new KTempDir( tmpDir() + "convtmp" );
    m_convert_tmpDir->setAutoDelete( true );
    connect( arch, SIGNAL( sigExtract( bool ) ), this, SLOT( convertSlotExtractDone( bool ) ) );
    m_convert_saveAsURL = u;
    arch->unarchFile( 0, m_convert_tmpDir->name() );
}

void
ArkWidget::convertSlotExtractDone( bool )
{
    kdDebug( 1601 ) << k_funcinfo << endl;
    disconnect( arch, SIGNAL( sigExtract( bool ) ), this, SLOT( convertSlotExtractDone( bool ) ) );
    QTimer::singleShot( 0, this, SLOT( convertSlotCreate() ) );
}

void
ArkWidget::convertSlotCreate()
{
    file_close();
    connect( this, SIGNAL( createDone( bool ) ), this, SLOT( convertSlotCreateDone( bool ) ) );
    QString archToCreate;
    if ( m_convert_saveAsURL.isLocalFile() )
        archToCreate = m_convert_saveAsURL.path();
    else
        archToCreate = tmpDir() + m_convert_saveAsURL.fileName();

    createArchive( archToCreate );
}


void
ArkWidget::convertSlotCreateDone( bool success )
{
    disconnect( this, SIGNAL( createDone( bool ) ), this, SLOT( convertSlotCreateDone( bool ) ) );
    kdDebug( 1601 ) << k_funcinfo << endl;
    if ( !success )
    {
        kdWarning( 1601 ) << "Error while converting. (convertSlotCreateDone)" << endl;
        return;
    }
    QDir dir( m_convert_tmpDir->name() );
    QStringList entries = dir.entryList();
    entries.remove( ".." );
    entries.remove( "." );
    QStringList::Iterator it = entries.begin();
    for ( ; it != entries.end(); ++it )
    {
        ///////////////////////////////////////////////////////
        // BIG TODO: get rid of 'the assume                  //
        // 'file:/', do some  black magic                    //
        // to find the basedir, chdir there,                 //
        // and break the rest of the world'                  //
        // hack. See also action_edit ...                    //
        // addFile should be:                                //
        // addFile( const QString & baseDir,                 //
        //          const QStringList & filesToAdd )         //
        //////////////////////////////////////////////////////
        *it = QString::fromLatin1( "file:" )+ m_convert_tmpDir->name() + *it;
    }
    bool bOldRecVal = ArkSettings::rarRecurseSubdirs();
    connect( arch, SIGNAL( sigAdd( bool ) ), this, SLOT( convertSlotAddDone( bool ) ) );
    arch->addFile( entries );
    ArkSettings::setRarRecurseSubdirs( bOldRecVal );
}

void
ArkWidget::convertSlotAddDone( bool success )
{
    disconnect( arch, SIGNAL( sigAdd( bool ) ), this, SLOT( convertSlotAddDone( bool ) ) );
    kdDebug( 1601 ) << k_funcinfo << endl;
    m_convertSuccess = success;
    // needed ? (TarArch, lzo)
    QTimer::singleShot( 0, this, SLOT( convertFinish() ) );
}

void
ArkWidget::convertFinish()
{
    kdDebug( 1601 ) << k_funcinfo << endl;
    delete m_convert_tmpDir;
    m_convert_tmpDir = NULL;

    ready();
    if ( m_convertSuccess )
    {
        if ( m_convert_saveAsURL.isLocalFile() )
        {
            emit openURLRequest( m_convert_saveAsURL );
        }
        else
        {
            KIO::NetAccess::upload( tmpDir()
                       + m_convert_saveAsURL.fileName(), m_convert_saveAsURL, this );
            // TODO: save bandwidth - we already have a local tmp file ...
            emit openURLRequest( m_convert_saveAsURL );
        }
    }
    else
    {
        kdWarning( 1601 ) << "Error while converting (convertSlotAddDone)" << endl;
    }
}

bool
ArkWidget::allowedArchiveName( const KURL & u )
{
    if (u.isEmpty())
        return false;

    QString archMimeType = KMimeType::findByURL( m_url )->name();
    if ( !m_openAsMimeType.isNull() )
        archMimeType = m_openAsMimeType;
    QString newArchMimeType = KMimeType::findByPath( u.path() )->name();
    if ( archMimeType == newArchMimeType )
        return true;

    return false;
}

void
ArkWidget::extractTo( const KURL & targetDirectory, const KURL & archive, bool bGuessName )
{
    m_extractTo_targetDirectory = targetDirectory;

    if ( bGuessName ) // suggest an extract directory based on archive name
    {
        const QString fileName = guessName( archive );
        m_extractTo_targetDirectory.setPath( targetDirectory.path( 1 ) + fileName + '/' );
    }

    if ( !KIO::NetAccess::exists( m_extractTo_targetDirectory, false, this ) )
    {
        if ( !KIO::NetAccess::mkdir( m_extractTo_targetDirectory, this ) )
        {
            KMessageBox::error( 0, i18n( "Could not create the folder %1" ).arg(
                                                            targetDirectory.prettyURL() ) );
            emit request_file_quit();
            return;
        }
    }

    connect( this, SIGNAL( openDone( bool ) ), this, SLOT( extractToSlotOpenDone( bool ) ) );
}

const QString
ArkWidget::guessName( const KURL &archive )
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
      fileName = fileName.left( fileName.findRev( ext ) );
      break;
    }
  }

  return fileName;
}

void
ArkWidget::extractToSlotOpenDone( bool success )
{
    disconnect( this, SIGNAL( openDone( bool ) ), this, SLOT( extractToSlotOpenDone( bool ) ) );
    if ( !success )
    {
        KMessageBox::error( this, i18n( "An error occurred while opening the archive %1." ).arg( m_url.prettyURL() ) );
        emit request_file_quit();
        return;
    }

    QString extractDir = m_extractTo_targetDirectory.path();
    // little code duplication from action_extract():
    if ( !m_extractTo_targetDirectory.isLocalFile() )
    {
        m_extractRemoteTmpDir = new KTempDir( tmpDir() + "extremote" );
        m_extractRemoteTmpDir->setAutoDelete( true );

        extractDir = m_extractRemoteTmpDir->name();
        m_extractRemote = true;

        if ( m_extractRemoteTmpDir->status() != 0 )
        {
            kdWarning(1601) << "Unable to create " << extractDir << endl;
            m_extractRemote = false;
            emit request_file_quit();
            return;
        }
    }

    QStringList empty;
    QStringList alreadyExisting = existingFiles( extractDir, empty );
    kdDebug( 1601 ) << "Already existing files count: " << existingFiles( extractDir, empty ).count() << endl;
    bool keepGoing = true;
    if ( !ArkSettings::extractOverwrite() && !alreadyExisting.isEmpty() )
    {
        keepGoing = ( KMessageBox::Continue == KMessageBox::warningContinueCancelList( this,
                    i18n( "The following files will not be extracted\nbecause they "
                          "already exist:" ), alreadyExisting ) );
    }

    if ( keepGoing ) // if the user's OK with those failures, go ahead
    {
        // unless we have no space!
        if ( ArkUtils::diskHasSpace( extractDir, m_nSizeOfFiles ) )
        {
            disableAll();
            connect( arch, SIGNAL( sigExtract( bool ) ), this, SLOT( extractToSlotExtractDone( bool ) ) );
            arch->unarchFile( 0, extractDir );
        }
        else
        {
            KMessageBox::error( this, i18n( "Not enough free disc space to extract the archive." ) );
            emit request_file_quit();
            return;
        }
    }
    else
        emit request_file_quit();
}

void
ArkWidget::extractToSlotExtractDone( bool success )
{
    disconnect( arch, SIGNAL( sigExtract( bool ) ), this, SLOT( extractToSlotExtractDone( bool ) ) );
    if ( !success )
    {
        kdDebug( 1601 ) << "Last Shell Output" << arch->getLastShellOutput() << endl;
        KMessageBox::error( this, i18n( "An error occurred while extracting the archive." ) );
        emit request_file_quit();
        return;
    }

    if (  m_extractRemote )
    {
        connect( this, SIGNAL( extractRemoteMovingDone() ), this, SIGNAL( request_file_quit() ) );
        extractRemoteInitiateMoving( m_extractTo_targetDirectory );
    }
    else
        emit request_file_quit();
}

bool
ArkWidget::addToArchive( const KURL::List & filesToAdd, const KURL & archive)
{
    m_addToArchive_filesToAdd = filesToAdd;
    m_addToArchive_archive = archive;
    if ( !KIO::NetAccess::exists( archive, false, this ) )
    {
        if ( !m_openAsMimeType.isEmpty() )
        {
            QStringList extensions = KMimeType::mimeType( m_openAsMimeType )->patterns();
            QStringList::Iterator it = extensions.begin();
            QString file = archive.path();
            for ( ; it != extensions.end() && !file.endsWith( ( *it ).remove( '*' ) ); ++it )
                ;

            if ( it == extensions.end() )
            {
                file += ArchiveFormatInfo::self()->defaultExtension( m_openAsMimeType );
                const_cast< KURL & >( archive ).setPath( file );
            }
        }

        connect( this, SIGNAL( createDone( bool ) ), this, SLOT( addToArchiveSlotCreateDone( bool ) ) );

        // TODO: remote Archives should be handled by createArchive
        if ( archive.isLocalFile() )
        {
            if ( !createArchive( archive.path() ) )
                 return false;
        }
        else
        {
            if ( !createArchive( tmpDir() + archive.fileName() ) )
                 return false;
        }
    return true;

    }
    connect( this, SIGNAL( openDone( bool ) ), this, SLOT( addToArchiveSlotOpenDone( bool ) ) );
    return true;
}

void
ArkWidget::addToArchiveSlotCreateDone( bool success )
{
    disconnect( this, SIGNAL( createDone( bool ) ), this, SLOT( addToArchiveSlotCreateDone( bool ) ) );
    if ( !success )
    {
        kdDebug( 1601 ) << "Could not create the archive" << endl;
        emit request_file_quit();
        return;
    }
    addToArchiveSlotOpenDone( true );
}

void
ArkWidget::addToArchiveSlotOpenDone( bool success )
{
    kdDebug( 1601 ) << k_funcinfo << endl;
    disconnect( this, SIGNAL( openDone( bool ) ), this, SLOT( addToArchiveSlotOpenDone( bool ) ) );
    // TODO: handle dirs with addDir ( or better+easier: get rid of the need to do that entirely )
    if ( !success )
    {
        emit request_file_quit();
        return;
    }

    if ( m_bIsSimpleCompressedFile && (m_nNumFiles == 1))
    {
        QString strFilename;
        KURL url = askToCreateRealArchive();
        strFilename = url.path();
        if (!strFilename.isEmpty())
        {
            connect( this, SIGNAL( createRealArchiveDone( bool ) ), this, SLOT( addToArchiveSlotAddDone( bool ) ) );
            createRealArchive( strFilename, m_addToArchive_filesToAdd.toStringList() );
            return;
        }
        else
        {
            emit request_file_quit();
            return;
        }
    }

/*    QStringList list = m_addToArchive_filesToAdd.toStringList();
    if ( !ArkUtils::diskHasSpace( tmpDir(), ArkUtils::getSizes( &list ) ) )
    {
        KMessageBox::error( this, i18n( "Not enough free disc space to extract the archive." ) );
        emit request_file_quit();
        return;
    }*/

    disableAll();
    // if they are URLs, we have to download them, replace the URLs
    // with filenames, and remember to delete the temporaries later.
/*    for ( QStringList::Iterator it = list.begin();
         it != list.end(); ++it)
    {
        QString str = *it;
        KURL url( toLocalFile( str ) );
        *it = url.prettyURL();
    }
*/
    KURL::List list = m_addToArchive_filesToAdd;


    // Remote URLs need to be downloaded.
    KURL::List::Iterator end( list.end() );
    for ( KURL::List::Iterator it = list.begin(); it != end; ++it )
    {
        if (!(*it).isLocalFile())
        {
            *it = toLocalFile( *it );
        }
    }

    kdDebug( 1601 ) << "Adding: " << list << endl;

    connect( arch, SIGNAL( sigAdd( bool ) ), this, SLOT( addToArchiveSlotAddDone( bool ) ) );
    arch->addFile( list.toStringList() );
}

void
ArkWidget::addToArchiveSlotAddDone( bool success )
{
    kdDebug( 1601 ) << k_funcinfo << endl;
    disconnect( this, SLOT( addToArchiveSlotAddDone( bool ) ) );
    if ( !success )
    {
        KMessageBox::error( this, i18n( "An error occurred while adding the files to the archive." ) );
    }
    if ( !m_addToArchive_archive.isLocalFile() )
        KIO::NetAccess::upload( m_strArchName, m_addToArchive_archive, this );
    emit request_file_quit();
    return;
}

void ArkWidget::setOpenAsMimeType( const QString & mimeType )
{
    m_openAsMimeType = mimeType;
}

void
ArkWidget::file_open(const KURL& url)
{
    if ( url.isEmpty() )
    {
        kdDebug( 1601 ) << "file_open: url empty" << endl;
        return;
    }

    if ( isArchiveOpen() )
        file_close();  // close old arch. If we don't, our temp file is wrong!

    if ( !url.isLocalFile() )
    {
        kdWarning ( 1601 ) << url.prettyURL() << " is not a local URL in ArkWidget::file_open( KURL). Aborting. " << endl;
        return;
    }


    QString strFile = url.path();

    kdDebug( 1601 ) << "File to open: " << strFile << endl;

    QFileInfo fileInfo( strFile );
    if ( !fileInfo.exists() )
    {
        KMessageBox::error(this, i18n("The archive %1 does not exist.").arg(strFile));
        emit removeRecentURL( m_realURL );
        return;
    }
    else if ( !fileInfo.isReadable() )
    {
        KMessageBox::error(this, i18n("You do not have permission to access that archive.") );
        emit removeRecentURL( m_realURL );
        return;
    }

    // see if the user is just opening the same file that's already
    // open (erm...)

    if (strFile == m_strArchName && m_bIsArchiveOpen)
    {
        kdDebug( 1601 ) << "file_open: strFile == m_strArchName" << endl;
        return;
    }

    // no errors if we made it this far.

    // Set the current archive filename to the filename
    m_strArchName = strFile;
    m_url = url;
    //arch->clearShellOutput();

    openArchive( strFile );
}


// File menu /////////////////////////////////////////////////////////

KURL
ArkWidget::getCreateFilename(const QString & _caption,
                                  const QString & _defaultMimeType,
                                  bool allowCompressed,
                                  const QString & _suggestedName )
{
    int choice=0;
    bool fileExists = true;
    QString strFile;
    KURL url;

    KFileDialog dlg( ":ArkSaveAsDialog", QString::null, this, "SaveAsDialog", true );
    dlg.setCaption( _caption );
    dlg.setOperationMode( KFileDialog::Saving );
    dlg.setMimeFilter( ArchiveFormatInfo::self()->supportedMimeTypes( allowCompressed ),
                       _defaultMimeType.isNull() ?  "application/x-tgz" : _defaultMimeType );
    if ( !_suggestedName.isEmpty() )
        dlg.setSelection( _suggestedName );

    while ( fileExists )
        // keep asking for filenames as long as the user doesn't want to
        // overwrite existing ones; break if they agree to overwrite
        // or if the file doesn't already exist. Return if they cancel.
        // Also check for proper extensions.
    {
        dlg.exec();
        url = dlg.selectedURL();
        strFile = url.path();

        if (strFile.isEmpty())
            return QString::null;

        //the user chose to save as the current archive
        //or wanted to create a new one with the same name
        //no need to do anything
        if (strFile == m_strArchName && m_bIsArchiveOpen)
            return QString::null;

        QStringList extensions = dlg.currentFilterMimeType()->patterns();
        QStringList::Iterator it = extensions.begin();
        for ( ; it != extensions.end() && !strFile.endsWith( ( *it ).remove( '*' ) ); ++it )
            ;

        if ( it == extensions.end() )
        {
            strFile += ArchiveFormatInfo::self()->defaultExtension( dlg.currentFilterMimeType()->name() );
            url.setPath( strFile );
        }

        kdDebug(1601) << "Trying to create an archive named " << strFile << endl;
        fileExists = QFile::exists( strFile );
        if( fileExists )
        {
            choice = KMessageBox::warningYesNoCancel(0,
               i18n("Archive already exists. Do you wish to overwrite it?"),
               i18n("Archive Already Exists"), i18n("Overwrite"), i18n("Do Not Overwrite"));

            if ( choice == KMessageBox::Yes )
            {
                QFile::remove( strFile );
                break;
            }
            else if ( choice == KMessageBox::Cancel )
            {
                return QString::null;
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
                      " to write to the directory %1" ).arg(url.directory() ) );
            return QString::null;
        }
    } // end of while loop

    return url;
}

void
ArkWidget::file_new()
{
    QString strFile;
    KURL url = getCreateFilename(i18n("Create New Archive") );
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
ArkWidget::slotExtractDone(bool success)
{
    disconnect( arch, SIGNAL( sigExtract( bool ) ),
                this, SLOT( slotExtractDone(bool) ) );
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

    if ( success && ArkSettings::openDestinationFolder() )
    {
            KRun::runURL( m_extractURL, "inode/directory" );
    }

    kdDebug(1601) << "-ArkWidget::slotExtractDone" << endl;
}

void
ArkWidget::extractRemoteInitiateMoving( const KURL & target )
{
    KURL srcDirURL;
    KURL src;
    QString srcDir;

    srcDir = m_extractRemoteTmpDir->name();
    srcDirURL.setPath( srcDir );

    QDir dir( srcDir );
    dir.setFilter( QDir::All | QDir::Hidden );
    QStringList lst( dir.entryList() );
    lst.remove( "." );
    lst.remove( ".." );

    KURL::List srcList;
    for( QStringList::ConstIterator it = lst.begin(); it != lst.end() ; ++it)
    {
        src = srcDirURL;
        src.addPath( *it );
        srcList.append( src );
    }

    m_extractURL.adjustPath( 1 );

    KIO::CopyJob *job = KIO::copy( srcList, target, this );
    connect( job, SIGNAL(result(KIO::Job*)),
            this, SLOT(slotExtractRemoteDone(KIO::Job*)) );

    m_extractRemote = false;
}

void
ArkWidget::slotExtractRemoteDone(KIO::Job *job)
{
    delete m_extractRemoteTmpDir;
    m_extractRemoteTmpDir = NULL;

    if ( job->error() )
        job->showErrorDialog();

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
        emit setWindowCaption( QString::null );
        emit removeOpenArk( m_strArchName );
        updateStatusTotals();
        updateStatusSelection();
        fixEnables();
    }
    else
    {
        closeArch();
    }

    m_strArchName = QString::null;
    m_url = KURL();
}


KURL
ArkWidget::askToCreateRealArchive()
{
    // ask user whether to create a real archive from a compressed file
    // returns filename if so
    KURL url;
    int choice =
        KMessageBox::warningYesNo(0, i18n("You are currently working with a simple compressed file.\nWould you like to make it into an archive so that it can contain multiple files?\nIf so, you must choose a name for your new archive."), i18n("Warning"),i18n("Make Into Archive"),i18n("Do Not Make"));
    if (choice == KMessageBox::Yes)
    {
        url = getCreateFilename( i18n("Create New Archive"),
                                 QString::null, false );
    }
    else
        url.setPath( QString::null );
    return url;
}

void
ArkWidget::createRealArchive( const QString & strFilename, const QStringList & filesToAdd )
{
    Arch * newArch = getNewArchive( strFilename );
    busy( i18n( "Creating archive..." ) );
    if ( !newArch )
        return;
    if ( !filesToAdd.isEmpty() )
        m_pTempAddList = new QStringList( filesToAdd );
    m_compressedFile = static_cast< CompressedFile * >( arch )->tempFileName();
    KURL u1, u2;
    u1.setPath( m_compressedFile );
    m_createRealArchTmpDir = new KTempDir( tmpDir() + "create_real_arch" );
    u2.setPath( m_createRealArchTmpDir->name() + u1.fileName() );
    KIO::NetAccess::copy( u1, u2, this );
    m_compressedFile = "file:" + u2.path(); // AGAIN THE 5 SPACES Hack :-(
    connect( newArch, SIGNAL( sigCreate( Arch *, bool, const QString &, int ) ),
             this, SLOT( createRealArchiveSlotCreate( Arch *, bool,
             const QString &, int ) ) );
    file_close();
    newArch->create();
}

void
ArkWidget::createRealArchiveSlotCreate( Arch * newArch, bool success,
                                             const QString & fileName, int nbr )
{
    slotCreate( newArch, success, fileName, nbr );

    if ( !success )
        return;

    QStringList listForCompressedFile;
    listForCompressedFile.append(m_compressedFile);
    disableAll();

    connect( newArch, SIGNAL( sigAdd( bool ) ), this,
                      SLOT( createRealArchiveSlotAddDone( bool ) ) );

    newArch->addFile(listForCompressedFile);
}

void
ArkWidget::createRealArchiveSlotAddDone( bool success )
{
    kdDebug( 1601 ) << "createRealArchiveSlotAddDone+, success:" << success << endl;
    disconnect( arch, SIGNAL( sigAdd( bool ) ), this,
                      SLOT( createRealArchiveSlotAddDone( bool ) ) );

    m_createRealArchTmpDir->unlink();
    delete m_createRealArchTmpDir;
    m_createRealArchTmpDir = NULL;


    if ( !success )
        return;

    ready();

    if ( m_pTempAddList == NULL )
    {
        // now get the files to be added
        // we don't know which files to add yet
        action_add();
    }
    else
    {
        connect( arch, SIGNAL( sigAdd( bool ) ), this,
                 SLOT( createRealArchiveSlotAddFilesDone( bool ) ) );
        // files were dropped in
        addFile( m_pTempAddList );
    }
}

void
ArkWidget::createRealArchiveSlotAddFilesDone( bool success )
{
    //kdDebug( 1601 ) << "createRealArchiveSlotAddFilesDone+, success:" << success << endl;
    disconnect( arch, SIGNAL( sigAdd( bool ) ), this,
                      SLOT( createRealArchiveSlotAddFilesDone( bool ) ) );
    delete m_pTempAddList;
    m_pTempAddList = NULL;
    emit createRealArchiveDone( success );
}




// Action menu /////////////////////////////////////////////////////////

void
ArkWidget::action_add()
{
    if (m_bIsSimpleCompressedFile && (m_nNumFiles == 1))
    {
        QString strFilename;
        KURL url = askToCreateRealArchive();
        strFilename = url.path();
        if (!strFilename.isEmpty())
        {
            createRealArchive(strFilename);
        }
        return;
    }

    KFileDialog fileDlg( ":ArkAddDir", QString::null, this, "adddlg", true );
    fileDlg.setMode( KFile::Mode( KFile::Files | KFile::ExistingOnly ) );
    fileDlg.setCaption(i18n("Select Files to Add"));

    if(fileDlg.exec())
    {
        KURL::List addList;
        addList = fileDlg.selectedURLs();
        QStringList * list = new QStringList();
        //Here we pre-calculate the end of the list
   KURL::List::ConstIterator endList = addList.end();
        for (KURL::List::ConstIterator it = addList.begin(); it != endList; ++it)
            list->append( KURL::decode_string( (*it).url() ) );

        if ( list->count() > 0 )
        {
            if ( m_bIsSimpleCompressedFile && list->count() > 1 )
            {
                QString strFilename;
                KURL url = askToCreateRealArchive();
                strFilename = url.path();
                if (!strFilename.isEmpty())
                {
                    createRealArchive(strFilename);
                }
                delete list;
                return;
            }
            addFile( list );
        }
        delete list;
    }
}

void
ArkWidget::addFile(QStringList *list)
{
    if ( !ArkUtils::diskHasSpace( tmpDir(), ArkUtils::getSizes( list ) ) )
        return;

    disableAll();
    busy( i18n( "Adding files..." ) );
    // if they are URLs, we have to download them, replace the URLs
    // with filenames, and remember to delete the temporaries later.
    for (QStringList::Iterator it = list->begin(); it != list->end(); ++it)
    {
        QString str = *it;
        *it = toLocalFile(KURL(str)).prettyURL();

    }

    connect( arch, SIGNAL( sigAdd( bool ) ), this, SLOT( slotAddDone( bool ) ) );
    arch->addFile( ( *list ) );
}

void
ArkWidget::action_add_dir()
{
    KURL u = KDirSelectDialog::selectDirectory( ":ArkAddDir",
                                                false, this,
                                                i18n("Select Folder to Add"));

    QString dir = KURL::decode_string( u.url(-1) );
    if ( !dir.isEmpty() )
    {
        busy( i18n( "Adding folder..." ) );
        disableAll();
        u = toLocalFile(u);
        connect( arch, SIGNAL( sigAdd( bool ) ), this, SLOT( slotAddDone( bool ) ) );
        arch->addDir( u.prettyURL() );
    }

}

void
ArkWidget::slotAddDone(bool _bSuccess)
{
    disconnect( arch, SIGNAL( sigAdd( bool ) ), this, SLOT( slotAddDone( bool ) ) );
    m_fileListView->setUpdatesEnabled(true);
    m_fileListView->triggerUpdate();
    ready();

    if (_bSuccess)
    {
        m_modified = true;
        //simulate reload
        KURL u;
        u.setPath( arch->fileName() );
        file_close();
        file_open( u );
        emit setWindowCaption( u.path() );
    }
    removeDownloadedFiles();
    fixEnables();
}



KURL
ArkWidget::toLocalFile( const KURL& url )
{
    KURL localURL = url;

    if(!url.isLocalFile())
    {
   QString strURL = url.prettyURL();

        QString tempfile = tmpDir();
        tempfile += strURL.right(strURL.length() - strURL.findRev("/") - 1);
        deleteAfterUse(tempfile);  // remember for deletion
        KURL tempurl; tempurl.setPath( tempfile );
        if( !KIO::NetAccess::dircopy(url, tempurl, this) )
            return KURL();
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
    if ( KMessageBox::warningContinueCancelList( this,
                                              i18n( "Do you really want to delete the selected items?" ),
                                              list,
                                              QString::null,
                                              KStdGuiItem::del(),
                                              "confirmDelete" )
         != KMessageBox::Continue)
    {
        return;
    }

    // Remove the entries from the list view
    QListViewItemIterator it( m_fileListView );
    while ( it.current() )
    {
        if ( it.current()->isSelected() )
            delete *it;
        else
            ++it;
    }

    disableAll();
    busy( i18n( "Removing..." ) );
    connect( arch, SIGNAL( sigDelete( bool ) ), this, SLOT( slotDeleteDone( bool ) ) );
    arch->remove(&list);
    kdDebug(1601) << "-ArkWidget::action_delete" << endl;
}

void
ArkWidget::slotDeleteDone(bool _bSuccess)
{
    disconnect( arch, SIGNAL( sigDelete( bool ) ), this, SLOT( slotDeleteDone( bool ) ) );
    kdDebug(1601) << "+ArkWidget::slotDeleteDone" << endl;
    m_fileListView->setUpdatesEnabled(true);
    m_fileListView->triggerUpdate();
    if (_bSuccess)
    {
        m_modified = true;
        updateStatusTotals();
        updateStatusSelection();
    }
    // disable the select all and extract options if there are no files left
    fixEnables();
    ready();
    kdDebug(1601) << "-ArkWidget::slotDeleteDone" << endl;

}



void
ArkWidget::slotOpenWith()
{
    connect( arch, SIGNAL( sigExtract( bool ) ), this,
            SLOT( openWithSlotExtractDone( bool ) ) );

    showCurrentFile();
}

void
ArkWidget::openWithSlotExtractDone( bool success )
{
    disconnect( arch, SIGNAL( sigExtract( bool ) ), this,
            SLOT( openWithSlotExtractDone( bool ) ) );

    if ( success )
    {
      KURL::List list;
      list.append(m_viewURL);
      KOpenWithDlg l( list, i18n("Open with:"), QString::null, (QWidget*)0L);
      if ( l.exec() )
      {
          KService::Ptr service = l.service();
          if ( !!service )
          {
              KRun::run( *service, list );
          }
          else
          {
              QString exec = l.text();
              exec += " %f";
              KRun::run( exec, list );
          }
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

    // Make sure to delete previous file already there...
    for(QStringList::ConstIterator it = fileList.begin();
        it != fileList.end(); ++it)
        QFile::remove(destTmpDirectory + *it);

    m_viewList = new QStringList( fileList );
    arch->unarchFile( m_viewList, destTmpDirectory, true);
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
    if (numFilesToReport != 0)
    {
        redoExtraction =  ( KMessageBox::Cancel == KMessageBox::warningContinueCancelList( this,
                    i18n( "The following files will not be extracted\nbecause they "
                          "already exist:" ), filesExisting ) );
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

        // if the filename ends with an "/", it means it is a directory
        if ( QFile::exists( strFullName ) && !strFilename.endsWith("/") )
        {
            existingFiles.append( strFilename );
        }
    }
    return existingFiles;
}





bool
ArkWidget::action_extract()
{
    KURL fileToExtract;
    fileToExtract.setPath( arch->fileName() );

     //before we start, make sure the archive is still there
    if (!KIO::NetAccess::exists( fileToExtract.prettyURL(), true, this ) )
    {
        KMessageBox::error(0, i18n("The archive to extract from no longer exists."));
        return false;
    }

    //if more than one entry in the archive is root level, suggest a path prefix
    QString prefix = m_fileListView->childCount() > 1 ?
                           QChar( '/' ) + guessName( realURL() )
                         : QString();

    // Should the extraction dialog show an option for extracting only selected files?
   bool enableSelected = ( m_nNumSelectedFiles > 0 ) &&
        ( m_fileListView->totalFiles() > 1);

    QString base = ArkSettings::extractionHistory().isEmpty()?
                       QString() : ArkSettings::extractionHistory().first();
    if ( base.isEmpty() )
    {
        // Perhaps the KDE Documents folder is a better choice?
        base = QDir::homeDirPath();
    }

    // Default URL shown in the extraction dialog;
    KURL defaultDir( base );

    if ( m_extractOnly )
    {
        defaultDir = KURL::fromPathOrURL( QDir::currentDirPath() );
    }

    ExtractionDialog *dlg = new ExtractionDialog( this, 0, enableSelected, defaultDir, prefix,  m_url.fileName() );

    bool bRedoExtract = false;

    // list of files to be extracted
    m_extractList = new QStringList;
    if ( dlg->exec() )
    {
        //m_extractURL will always be the location the user chose to
        //m_extract to, whether local or remote
        m_extractURL = dlg->extractionDirectory();

        //extractDir will either be the real, local extract dir,
        //or in case of a extract to remote location, a local tmp dir
        QString extractDir;

        if ( !m_extractURL.isLocalFile() )
        {
            m_extractRemoteTmpDir = new KTempDir( tmpDir() + "extremote" );
            m_extractRemoteTmpDir->setAutoDelete( true );

            extractDir = m_extractRemoteTmpDir->name();
            m_extractRemote = true;
            if ( m_extractRemoteTmpDir->status() != 0 )
            {
                kdWarning( 1601 ) << "Unable to create temporary directory" << extractDir << endl;
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
                    connect( arch, SIGNAL( sigExtract( bool ) ), this, SLOT( slotExtractDone(bool) ) );
                    arch->unarchFile(0, extractDir);
                }
            }
        }
        else
        {
                KIO::filesize_t nTotalSize = 0;
                // make a list to send to unarchFile
                QStringList selectedFiles = m_fileListView->selectedFilenames();
                for ( QStringList::const_iterator it = selectedFiles.constBegin();
                      it != selectedFiles.constEnd();
                      ++it )
                {
                    m_extractList->append( QFile::encodeName( *it ) );
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
                                 this, SLOT( slotExtractDone(bool) ) );
                        arch->unarchFile(m_extractList, extractDir); // extract selected files
                    }
                }
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
ArkWidget::action_edit()
{
    // begin an edit. This is like a view, but once the process exits,
    // the file is put back into the archive. If the user tries to quit or
    // close the archive, there will be a warning that any changes to the
    // files open under "Edit" will be lost unless the archive remains open.
    // [hmm, does that really make sense? I'll leave it for now.]

    busy( i18n( "Extracting..." ) );
    connect( arch, SIGNAL( sigExtract( bool ) ), this,
                        SLOT( editSlotExtractDone() ) );
    showCurrentFile();
}

void
ArkWidget::editSlotExtractDone()
{
    disconnect( arch, SIGNAL( sigExtract( bool ) ),
                this, SLOT( editSlotExtractDone() ) );
    ready();
    editStart();

    // avoid race condition, don't do updates if application is exiting
    if( m_fileListView )
    {
        m_fileListView->setUpdatesEnabled(true);
        fixEnables();
    }
}

void
ArkWidget::editStart()
{
    kdDebug(1601) << "Edit in progress..." << endl;
    KURL::List list;
    // edit will be in progress until the KProcess terminates.
    KOpenWithDlg l( list, i18n("Edit with:"),
            QString::null, (QWidget*)0L );
    if ( l.exec() )
    {
        KProcess *kp = new KProcess;

        *kp << l.text() << m_strFileToView;
        connect( kp, SIGNAL(processExited(KProcess *)),
                this, SLOT(slotEditFinished(KProcess *)) );
        if ( kp->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false )
        {
            KMessageBox::error(0, i18n("Trouble editing the file..."));
        }
    }
}

void
ArkWidget::slotEditFinished(KProcess *kp)
{
    kdDebug(1601) << "+ArkWidget::slotEditFinished" << endl;
    connect( arch, SIGNAL( sigAdd( bool ) ), this, SLOT( editSlotAddDone( bool ) ) );
    delete kp;
    QStringList list;
    // now put the file back into the archive.
    list.append(m_strFileToView);
    disableAll();


    // BUG: this puts any edited file back at the archive toplevel...
    // there's only one file, and it's in the temp directory.
    // If the filename has more than three /'s then we should
    // change to the first level directory so that the paths
    // come out right.
    QStringList::Iterator it = list.begin();
    QString filename = *it;
    QString path;
    if (filename.contains('/') > 3)
    {
        kdDebug(1601) << "Filename is originally: " << filename << endl;
        int i = filename.find('/', 5);
        path = filename.left(1+i);
        kdDebug(1601) << "Changing to dir: " << path << endl;
        QDir::setCurrent(path);
        filename = filename.right(filename.length()-i-1);
        // HACK!! We need a relative path. If I have "file:", it
        // will look like an absolute path. So five spaces here to get
        // chopped off later....
        filename = "     " + filename;
        *it = filename;
    }

    busy( i18n( "Readding edited file..." ) );
    arch->addFile( list );

    kdDebug(1601) << "-ArkWidget::slotEditFinished" << endl;
}

void
ArkWidget::editSlotAddDone( bool success )
{
    ready();
    disconnect( arch, SIGNAL( sigAdd( bool ) ), this, SLOT( editSlotAddDone( bool ) ) );
    slotAddDone( success );
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
            ArkViewer * viewer = new ArkViewer( this, "viewer" );

            if ( !viewer->view( m_viewURL ) )
            {
                QString text = i18n( "The internal viewer is not able to display this file. Would you like to view it using an external program?" );
                view = ( KMessageBox::warningYesNo( this, text, QString::null, i18n("View Externally"), i18n("Do Not View") ) == KMessageBox::Yes );

                if ( view )
                    viewInExternalViewer( this, m_viewURL );
            }
        }
        else
        {
            viewInExternalViewer( this, m_viewURL );
        }
    }

    disconnect( arch, SIGNAL( sigExtract( bool ) ), this,
                SLOT( viewSlotExtractDone( bool ) ) );

    delete m_viewList;

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

    QString name = m_fileListView->currentItem()->fileName();

    QString fullname = tmpDir();
    fullname += name;

    if(fullname.contains("../"))
        fullname.remove("../");

    //Convert the QString filename to KURL to escape the bad characters
    m_viewURL.setPath(fullname);

    m_strFileToView = fullname;
    kdDebug(1601) << "File to be extracted: " << m_viewURL << endl;

    QStringList extractList;
    extractList.append(name);

    if (ArkUtils::diskHasSpace( tmpDir(), m_fileListView->currentItem()->fileSize() ) )
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
ArkWidget::doPopup( QListViewItem *pItem, const QPoint &pPoint, int nCol ) // slot
// do the right-click popup menus
{
    if ( nCol == 0 || !m_bArchivePopupEnabled )
    {
        m_fileListView->setCurrentItem(pItem);
        m_fileListView->setSelected(pItem, true);
        emit signalFilePopup( pPoint );
    }
    else // clicked anywhere else but the name column
    {
        emit signalArchivePopup( pPoint );
    }
}


void
ArkWidget::viewFile( QListViewItem* item ) // slot
// show contents when double click
{
    // Preview, if it is a file
    if ( item->childCount() == 0)
        emit action_view();
    else  // Change opened state if it is a dir
        item->setOpen( !item->isOpen() );
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
    else if (m_nNumSelectedFiles != 1)
    {
        strInfo = i18n("%1 files selected  %2")
                  .arg(KGlobal::locale()->formatNumber(m_nNumSelectedFiles, 0))
                  .arg(KIO::convertSize(m_nSizeOfSelectedFiles));
    }
    else
    {
        strInfo = i18n("1 file selected  %2")
                  .arg(KIO::convertSize(m_nSizeOfSelectedFiles));
    }

    emit setStatusBarSelectedFiles(strInfo);
    fixEnables();
}


// Drag & Drop ////////////////////////////////////////////////////////

void
ArkWidget::dragMoveEvent(QDragMoveEvent *e)
{
    if (KURLDrag::canDecode(e) && !m_bDropSourceIsSelf)
    {
        e->accept();
    }
}


void
ArkWidget::dropEvent(QDropEvent* e)
{
    kdDebug( 1601 ) << "+ArkWidget::dropEvent" << endl;

    KURL::List list;

    if ( KURLDrag::decode( e, list ) )
    {
        QStringList urlList = list.toStringList();
        dropAction( urlList );
    }

    kdDebug(1601) << "-dropEvent" << endl;
}

//////////////////////////////////////////////////////////////////////
///////////////////////// dropAction /////////////////////////////////
//////////////////////////////////////////////////////////////////////

void
ArkWidget::dropAction( QStringList  & list )
{
    // Called by dropEvent

    // The possibilities treated are as follows:
    // drop a regular file into a window with
    //   * an open archive - add it.
    //   * no open archive - ask user to open an archive for adding file or cancel
    // drop an archive into a window with
    //   * an open archive - ask user to add to open archive or to open it freshly
    //   * no open archive - open it
    // drop many files (can be a mix of archives and regular) into a window with
    //   * an open archive - add them.
    //   * no open archive - ask user to open an archive for adding files or cancel

    // and don't forget about gzip files.

    QString str;
    QStringList urls; // to be sent to addFile

    str = list.first();

    if ( 1 == list.count() &&
         ( UNKNOWN_FORMAT != ArchiveFormatInfo::self()->archTypeByExtension( str ) ) )
    {
        // if there's one thing being dropped and it's an archive
        if (isArchiveOpen())
        {
            // ask them if they want to add the dragged archive to the current
            // one or open it as the new current archive
            int nRet = KMessageBox::warningYesNoCancel(this,
                       i18n("Do you wish to add this to the current archive or open it as a new archive?"),
                       QString::null,
                       i18n("&Add"), i18n("&Open"));
            if (KMessageBox::Yes == nRet) // add it
            {
                if (m_bIsSimpleCompressedFile && (m_nNumFiles == 1))
                {
                    QString strFilename;
                    KURL url = askToCreateRealArchive();
                    strFilename = url.path();
                    if (!strFilename.isEmpty())
                    {
                        createRealArchive( strFilename, list );
                    }
                    return;
                }

                addFile( &list );
                return;
            }
            else if (KMessageBox::Cancel == nRet) // cancel
            {
                return;
            }
        }

        // if I made it here, there's either no archive currently open
        // or they selected "Open".
        KURL url = str;

        emit openURLRequest( url );
    }
    else
    {
        if (isArchiveOpen())
        {
            if (m_bIsSimpleCompressedFile && (m_nNumFiles == 1))
            {
                QString strFilename;
                KURL url = askToCreateRealArchive();
                strFilename = url.path();
                if (!strFilename.isEmpty())
                {
                    createRealArchive( strFilename, list );
                }
                return;
            }
            // add the files to the open archive
            addFile( &list );
        }
        else
        {
            // no archive is open, so we ask if the user wants to open one
            // for this/these file/files.

            QString str;
            str = (list.count() > 1)
                  ? i18n("There is no archive currently open. Do you wish to create one now for these files?")
                  : i18n("There is no archive currently open. Do you wish to create one now for this file?");
            int nRet = KMessageBox::warningYesNo(this, str, QString::null, i18n("Create Archive"), i18n("Do Not Create"));
            if (nRet == KMessageBox::Yes) // yes
            {
                file_new();
                if (isArchiveOpen()) // they still could have canceled!
                {
                    addFile( &list );
                }
            }
            // else // basically a cancel on the drop.
        }
    }
}

void
ArkWidget::startDrag( const QStringList & fileList )
{
    mDragFiles = fileList;
    connect( arch, SIGNAL( sigExtract( bool ) ), this, SLOT( startDragSlotExtractDone( bool ) ) );
    prepareViewFiles( fileList );
}

void
ArkWidget::startDragSlotExtractDone( bool )
{
    disconnect( arch, SIGNAL( sigExtract( bool ) ),
                this, SLOT( startDragSlotExtractDone( bool ) ) );

    KURL::List list;

    for (QStringList::Iterator it = mDragFiles.begin(); it != mDragFiles.end(); ++it)
    {
        KURL url;
        url.setPath( tmpDir() + *it );
        list.append( url );
    }

    KURLDrag *drg = new KURLDrag(list, m_fileListView->viewport(), "Ark Archive Drag" );
    m_bDropSourceIsSelf = true;
    drg->dragCopy();
    m_bDropSourceIsSelf = false;
}


void
ArkWidget::arkWarning(const QString& msg)
{
    KMessageBox::information(this, msg);
}

void
ArkWidget::createFileListView()
{
   kdDebug(1601) << "ArkWidget::createFileListView" << endl;
   if ( !m_fileListView )
   {
      m_fileListView = new FileListView(this);

      connect( m_fileListView, SIGNAL( selectionChanged() ),
               this, SLOT( slotSelectionChanged() ) );
      connect( m_fileListView, SIGNAL( rightButtonPressed(QListViewItem *, const QPoint &, int) ),
            this, SLOT(doPopup(QListViewItem *, const QPoint &, int)));
      connect( m_fileListView, SIGNAL( startDragRequest( const QStringList & ) ),
            this, SLOT( startDrag( const QStringList & ) ) );
      connect( m_fileListView, SIGNAL( executed(QListViewItem *, const QPoint &, int ) ),
            this, SLOT( viewFile(QListViewItem*) ) );
      connect( m_fileListView, SIGNAL( returnPressed(QListViewItem * ) ),
            this, SLOT( viewFile(QListViewItem*) ) );
    }
    m_fileListView->clear();
}


Arch * ArkWidget::getNewArchive( const QString & _fileName, const QString& _mimetype )
{
    Arch * newArch = 0;

    QString type = _mimetype.isNull()? KMimeType::findByURL( KURL::fromPathOrURL(_fileName) )->name() : _mimetype;
    ArchType archtype = ArchiveFormatInfo::self()->archTypeForMimeType(type);
    kdDebug( 1601 ) << "archtype is recognised as: " << archtype << endl;
    if(0 == (newArch = Arch::archFactory(archtype, this,
                                         _fileName, _mimetype)))
    {
        KMessageBox::error(this, i18n("Unknown archive format or corrupted archive") );
        emit request_file_quit();
        return NULL;
    }

    if (!newArch->archUtilityIsAvailable())
    {
        KMessageBox::error(this, i18n("The utility %1 is not in your PATH.\nPlease install it or contact your system administrator.").arg(newArch->getArchUtility()));
        return NULL;
    }

    connect( newArch, SIGNAL(headers(const ColumnList&)),
             m_fileListView, SLOT(setHeaders(const ColumnList&)));

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
    kdDebug( 1601 ) << k_funcinfo << endl;
    disconnect( _newarch, SIGNAL( sigCreate( Arch *, bool, const QString &, int ) ),
                this, SLOT(slotCreate(Arch *, bool, const QString &, int) ) );
    ready();
    if ( _success )
    {
        //file_close(); already called in ArkWidget::file_new()
        m_strArchName = _filename;
        // for the hack in compressedfile; needed when creating a compressedfile
        // then directly adding a file
        KURL u;
        u.setPath( _filename );
        setRealURL( u );

        emit setWindowCaption( _filename );
        emit addRecentURL( u );
        createFileListView();
   m_fileListView->show();
        m_bIsArchiveOpen = true;
        arch = _newarch;
        m_bIsSimpleCompressedFile =
            (m_archType == COMPRESSED_FORMAT);
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
    Arch *newArch = 0;
    ArchType archtype;
    ArchiveFormatInfo * info = ArchiveFormatInfo::self();
    if ( m_openAsMimeType.isNull() )
    {
        archtype = info->archTypeForURL( m_url );
        if ( info->wasUnknownExtension() )
        {
            ArchiveFormatDlg * dlg = new ArchiveFormatDlg( this, info->findMimeType( m_url ) );
            if ( !dlg->exec() == QDialog::Accepted )
            {
                emit setWindowCaption( QString::null );
                emit removeRecentURL( m_realURL );
                delete dlg;
                file_close();
                return;
            }
            m_openAsMimeType = dlg->mimeType();
            archtype = info->archTypeForMimeType( m_openAsMimeType );
            delete dlg;
        }
    }
    else
    {
       archtype = info->archTypeForMimeType( m_openAsMimeType );
    }

    kdDebug( 1601 ) << "m_openAsMimeType is: " << m_openAsMimeType << endl;
    if( 0 == ( newArch = Arch::archFactory( archtype, this,
                                            _filename, m_openAsMimeType) ) )
    {
        emit setWindowCaption( QString::null );
        emit removeRecentURL( m_realURL );
        KMessageBox::error( this, i18n("Unknown archive format or corrupted archive") );
        return;
    }

    if (!newArch->unarchUtilityIsAvailable())
    {
        KMessageBox::error(this, i18n("The utility %1 is not in your PATH.\nPlease install it or contact your system administrator.").arg(newArch->getUnarchUtility()));
        return;
    }

    m_archType = archtype;

    connect( newArch, SIGNAL(sigOpen(Arch *, bool, const QString &, int)),
             this, SLOT(slotOpen(Arch *, bool, const QString &,int)) );
    connect( newArch, SIGNAL(headers(const ColumnList&)),
             m_fileListView, SLOT(setHeaders(const ColumnList&)));

    disableAll();

    busy( i18n( "Opening the archive..." ) );
    m_fileListView->setUpdatesEnabled( false );
    arch = newArch;
    newArch->open();
    emit addRecentURL( m_url );
}

void
ArkWidget::slotOpen( Arch * /* _newarch */, bool _success, const QString & _filename, int )
{
    ready();
    m_fileListView->setUpdatesEnabled(true);
    m_fileListView->triggerUpdate();

    m_fileListView->show();

    if ( _success )
    {
        QFileInfo fi( _filename );
        QString path = fi.dirPath( true );

        if ( !fi.isWritable() )
        {
            arch->setReadOnly(true);
            KMessageBox::information(this, i18n("This archive is read-only. If you want to save it under a new name, go to the File menu and select Save As."), i18n("Information"), "ReadOnlyArchive");
        }
        updateStatusTotals();
        m_bIsArchiveOpen = true;
        m_bIsSimpleCompressedFile = ( m_archType == COMPRESSED_FORMAT );

        if ( m_extractOnly )
        {
            extractOnlyOpenDone();
            return;
        }
        m_fileListView->adjustColumns();
        emit addOpenArk( _filename );
    }
    else
    {
        emit removeRecentURL( m_realURL );
        emit setWindowCaption( QString::null );
        KMessageBox::error( this, i18n( "An error occurred while trying to open the archive %1" ).arg( _filename ) );

        if ( m_extractOnly )
            emit request_file_quit();
    }

    fixEnables();
    emit openDone( _success );
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
void ArkWidget::showSettings(){
  if(KConfigDialog::showDialog("settings"))
    return;

  KConfigDialog *dialog = new KConfigDialog(this, "settings", ArkSettings::self());

  General* genPage = new General(0, "General");
  dialog->addPage(genPage, i18n("General"), "ark", i18n("General Settings"));
  dialog->addPage(new Addition(0, "Addition"), i18n("Addition"), "ark_addfile", i18n("File Addition Settings"));
  dialog->addPage(new Extraction(0, "Extraction"), i18n("Extraction"), "ark_extract", i18n("Extraction Settings"));

  KTrader::OfferList offers;

  offers = KTrader::self()->query( "KonqPopupMenu/Plugin", "Library == 'libarkplugin'" );

  if ( offers.isEmpty() )
     genPage->kcfg_KonquerorIntegration->setEnabled( false );
  else
     genPage->konqIntegrationLabel->setText( QString::null );

  dialog->show();
}

#include "arkwidget.moc"
