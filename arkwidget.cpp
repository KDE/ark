/*
 Emacs: -*- mode: c++; c-basic-offset: 4; -*-

 ark -- archiver for the KDE project

 Copyright (C)

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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <sys/types.h>
#include <sys/stat.h>

// Qt includes
#include <qlayout.h>

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

// ark includes
#include "arkapp.h"
#include "generalOptDlg.h"
#include "selectDlg.h"
#include "archiveformatdlg.h"
#include "extractdlg.h"
#include "arkwidget.h"
#include "filelistview.h"
#include "arksettings.h"
#include "arkutils.h"
#include "archiveformatinfo.h"
#include "compressedfile.h"

//----------------------------------------------------------------------
//
//  Class ArkWidget starts here
//
//----------------------------------------------------------------------

ArkWidget::ArkWidget( QWidget *parent, const char *name ) :
        QWidget(parent, name), ArkWidgetBase(this),
        m_bBusy( false ), m_bBusyHold( false ), m_extractOnly(false),
        m_extractRemote(false), m_openAsMimeType(QString::null),
        m_pTempAddList(NULL), mpDownloadedList(NULL),
        m_bArchivePopupEnabled( false ), m_convert_tmpDir( NULL ),
        m_convertSuccess( false ), m_createRealArchTmpDir( NULL ),
        m_extractRemoteTmpDir( NULL ), m_modified( false )
{
    kdDebug(1601) << "+ArkWidget::ArkWidget" << endl;
    QHBoxLayout * l = new QHBoxLayout( this );
    l->setAutoAdd( true );
    createFileListView();
    // enable DnD
    setAcceptDrops(true);
    kdDebug(1601) << "-ArkWidget::ArkWidget" << endl;
}

ArkWidget::~ArkWidget()
{
    cleanArkTmpDir();
    ready();
    kdDebug(1601) << "-ArkWidget::~ArkWidget" << endl;
}


////////////////////////////////////////////////////////////////////
///////////////////////// updateStatusTotals ///////////////////////
////////////////////////////////////////////////////////////////////

void 
ArkWidget::updateStatusTotals()
{
    m_nNumFiles = 0;
    m_nSizeOfFiles = 0;
    if (archiveContent)
    {
        FileLVI *pItem = (FileLVI *)archiveContent->firstChild();
        while (pItem)
        {
            ++m_nNumFiles;
            m_nSizeOfFiles += pItem->fileSize();
            pItem = (FileLVI *)pItem->nextSibling();
        }
    }

    QString strInfo = i18n("%n file  %1", "%n files  %1", m_nNumFiles).arg(KIO::convertSize(m_nSizeOfFiles));
    emit setStatusBarText(strInfo);
}

void 
ArkWidget::busy( const QString & text )
{
    kdDebug( 1601 ) << "ArkWidget::busy()+" << endl;
    emit setBusy( text );

    if ( m_bBusy )
        return;

    QApplication::setOverrideCursor( waitCursor );
    m_bBusy = true;
    kdDebug( 1601 ) << "ArkWidget::busy()-" << endl;
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
    kdDebug( 1601 ) << "ArkWidget::ready()+" << endl;
    if ( !m_bBusy )
        return;

    QApplication::restoreOverrideCursor();
    emit setReady();
    m_bBusyHold = false;
    m_bBusy = false;
    kdDebug( 1601 ) << "ArkWidget::ready()-" << endl;
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
    do
    {
        u = getCreateFilename( i18n( "Save Archive As" ), defaultMimeType );
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
    m_convert_tmpDir =  new KTempDir( m_settings->getTmpDir() + "convtmp" );
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
        archToCreate = m_settings->getTmpDir() + m_convert_saveAsURL.fileName();

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
    bool bOldRecVal = m_settings->getZipAddRecurseDirs();
    connect( arch, SIGNAL( sigAdd( bool ) ), this, SLOT( convertSlotAddDone( bool ) ) );
    arch->addFile( &entries );
    m_settings->setZipAddRecurseDirs( bOldRecVal );
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
            KIO::NetAccess::upload( m_settings->getTmpDir()
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
        m_extractRemoteTmpDir = new KTempDir( m_settings->getTmpDir() + "extremote" );
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
    if ( !m_settings->getExtractOverwrite() && !alreadyExisting.isEmpty() )
    {
       if ( alreadyExisting.count() == m_nNumFiles )
        {
            KMessageBox::error( this, i18n( "None of the files in the archive have been\n"
                                            "extracted since all of them already exist.") );
            emit request_file_quit();
            return;
        }
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
}

void 
ArkWidget::extractToSlotExtractDone( bool success )
{
    disconnect( arch, SIGNAL( sigExtract( bool ) ), this, SLOT( extractToSlotExtractDone( bool ) ) );
    if ( !success )
    {
        kdDebug( 1601 ) << "Last Shell Output" << *( m_settings->getLastShellOutput() ) << endl;
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

void
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
            createArchive( archive.path() );
        else
            createArchive( m_settings->getTmpDir() + archive.fileName() );


        return;
    }
    connect( this, SIGNAL( openDone( bool ) ), this, SLOT( addToArchiveSlotOpenDone( bool ) ) );
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

    QStringList list = m_addToArchive_filesToAdd.toStringList();
    if ( !ArkUtils::diskHasSpace( m_strArchName, ArkUtils::getSizes( &list ) ) )
    {
        KMessageBox::error( this, i18n( "Not enough free disc space to extract the archive." ) );
        emit request_file_quit();
        return;
    }

    disableAll();
    // if they are URLs, we have to download them, replace the URLs
    // with filenames, and remember to delete the temporaries later.
    for ( QStringList::Iterator it = list.begin();
         it != list.end(); ++it)
    {
        QString str = *it;
        KURL url( toLocalFile( str ) );
        *it = url.prettyURL();
    }

    kdDebug( 1601 ) << "Adding: " << list << endl;

    connect( arch, SIGNAL( sigAdd( bool ) ), this, SLOT( addToArchiveSlotAddDone( bool ) ) );
    arch->addFile( &list );
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
    kdDebug(1601) << "+ArkWidget::file_open(const KURL& url)" << endl;
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
    m_settings->clearShellOutput();

    openArchive( strFile );

    kdDebug(1601) << "-ArkWidget::file_open(const KURL& url)" << endl;
}


// File menu /////////////////////////////////////////////////////////

KURL 
ArkWidget::getCreateFilename(const QString & _caption,
                                  const QString & _defaultMimeType,
                                  bool allowCompressed)
{
    int choice=0;
    bool fileExists = true;
    QString strFile;
    KURL url;

    KFileDialog dlg( ":ArkSaveAsDialog", QString::null, this, "SaveAsDialog", true );
    dlg.setCaption( _caption );
    dlg.setOperationMode( KFileDialog::Saving );
    dlg.setFilterMimeType( i18n( "Archive &format:" ),
           ArchiveFormatInfo::self()->supportedMimeTypes( allowCompressed ),
           _defaultMimeType.isNull() ? KMimeType::mimeType( "application/x-tgz" )
                                     : KMimeType::mimeType( _defaultMimeType ) );

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
               i18n("Archive Already Exists"));

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
        m_settings->clearShellOutput();
        file_close();
        createArchive( strFile );
    }
}

void 
ArkWidget::extractOnlyOpenDone()
{
    int oldMode = m_settings->getExtractDirMode();
    QString oldFixedExtractDir = m_settings->getFixedExtractDir();
    m_settings->setExtractDirCfg( m_url.upURL().path(), ArkSettings::FIXED_EXTRACT_DIR );
    bool done = action_extract();
    // Extract should have started before this returns, so hopefully safe.
    m_settings->setExtractDirCfg( oldFixedExtractDir, oldMode );
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

    if( archiveContent ) // avoid race condition, don't do updates if application is exiting
    {
        archiveContent->setUpdatesEnabled(true);
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
    kdDebug(1601) << "+ArkWidget::disableAll" << endl;
    emit disableAllActions();
    archiveContent->setUpdatesEnabled(true);
    kdDebug(1601) << "-ArkWidget::disableAll" << endl;
}

void 
ArkWidget::fixEnables() // private
{
    emit fixActions(); //connected to the part
}

void
ArkWidget::file_close()
{
    kdDebug(1601) << "+ArkWidget::file_close" << endl;
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

    kdDebug(1601) << "-ArkWidget::file_close" << endl;
}


// Edit menu /////////////////////////////////////////////////////////

void 
ArkWidget::edit_select()
{
    SelectDlg *sd = new SelectDlg( m_settings, this );
    if ( sd->exec() )
    {
        QString exp = sd->getRegExp();
        m_settings->setSelectRegExp( exp );

        QRegExp reg_exp( exp, true, true );
        if (!reg_exp.isValid())
            kdError(1601) <<
            "ArkWidget::edit_select: regular expression is not valid." << endl;
        else
        {
            // first deselect everything
            archiveContent->clearSelection();
            FileLVI * flvi = (FileLVI*)archiveContent->firstChild();


            // don't call the slot for each selection!
            disconnect( archiveContent, SIGNAL( selectionChanged()),
                        this, SLOT( slotSelectionChanged() ) );

            while (flvi)
            {
                if ( reg_exp.search(flvi->fileName())==0 )
                {
                    archiveContent->setSelected(flvi, true);
                }
                flvi = (FileLVI*)flvi->itemBelow();
            }
            // restore the behavior
            connect( archiveContent, SIGNAL( selectionChanged()),
                     this, SLOT( slotSelectionChanged() ) );
            updateStatusSelection();
        }
    }
}

void 
ArkWidget::edit_selectAll()
{
    FileLVI * flvi = (FileLVI*)archiveContent->firstChild();

    // don't call the slot for each selection!
    disconnect( archiveContent, SIGNAL( selectionChanged()),
                this, SLOT( slotSelectionChanged() ) );
    while (flvi)
    {
        archiveContent->setSelected(flvi, true);
        flvi = (FileLVI*)flvi->itemBelow();
    }

    // restore the behavior
    connect( archiveContent, SIGNAL( selectionChanged()),
             this, SLOT( slotSelectionChanged() ) );
    updateStatusSelection();
}

void 
ArkWidget::edit_deselectAll()
{
    archiveContent->clearSelection();
    updateStatusSelection();
}

void 
ArkWidget::edit_invertSel()
{
    FileLVI * flvi = (FileLVI*)archiveContent->firstChild();
    // don't call the slot for each selection!
    disconnect( archiveContent, SIGNAL( selectionChanged()),
                this, SLOT( slotSelectionChanged() ) );

    while (flvi)
    {
        archiveContent->setSelected(flvi, !flvi->isSelected());
        flvi = (FileLVI*)flvi->itemBelow();
    }
    // restore the behavior
    connect( archiveContent, SIGNAL( selectionChanged()),
             this, SLOT( slotSelectionChanged() ) );
    updateStatusSelection();
}

void 
ArkWidget::edit_view_last_shell_output()
{
    viewShellOutput();
}

KURL 
ArkWidget::askToCreateRealArchive()
{
    // ask user whether to create a real archive from a compressed file
    // returns filename if so
    KURL url;
    int choice =
        KMessageBox::warningYesNo(0, i18n("You are currently working with a simple compressed file.\nWould you like to make it into an archive so that it can contain multiple files?\nIf so, you must choose a name for your new archive."), i18n("Warning"));
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
    m_createRealArchTmpDir = new KTempDir( m_settings->getTmpDir() + "create_real_arch" );
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

    newArch->addFile(&listForCompressedFile);
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
    kdDebug(1601) << "Add dir: " << m_settings->getAddDir() << endl;

    KFileDialog fileDlg( m_settings->getAddDir(), QString::null, this, "adddlg", true );
    fileDlg.setMode( KFile::Mode( KFile::Files | KFile::ExistingOnly ) );
    fileDlg.setCaption(i18n("Select Files to Add"));

    if(fileDlg.exec())
    {
        KURL::List addList;
        addList = fileDlg.selectedURLs();
        QStringList * list = new QStringList();
        for (KURL::List::ConstIterator it = addList.begin(); it != addList.end(); it++)
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
            delete list;
        }
    }
}

void 
ArkWidget::addFile(QStringList *list)
{
    if ( !ArkUtils::diskHasSpace( m_strArchName, ArkUtils::getSizes( list ) ) )
        return;

    disableAll();
    busy( i18n( "Adding files..." ) );
    // if they are URLs, we have to download them, replace the URLs
    // with filenames, and remember to delete the temporaries later.
    for (QStringList::Iterator it = list->begin(); it != list->end(); ++it)
    {
        QString str = *it;
        //kdDebug(1601) << "Want to add " << str<< endl;
        KURL url( toLocalFile(str) );
        *it = url.prettyURL();

    }

    connect( arch, SIGNAL( sigAdd( bool ) ), this, SLOT( slotAddDone( bool ) ) );
    arch->addFile(list);
}

void 
ArkWidget::action_add_dir()
{
    KURL u = KDirSelectDialog::selectDirectory( m_settings->getAddDir(),
                                                    false, this,
                                                    i18n("Select Folder to Add"));

    QString dir = KURL::decode_string( u.url(-1) );
    if ( !dir.isEmpty() )
    {
        busy( i18n( "Adding folder..." ) );
        disableAll();
        u = toLocalFile(dir);
        connect( arch, SIGNAL( sigAdd( bool ) ), this, SLOT( slotAddDone( bool ) ) );
        arch->addDir( u.prettyURL() );
    }

}

void 
ArkWidget::slotAddDone(bool _bSuccess)
{
    disconnect( arch, SIGNAL( sigAdd( bool ) ), this, SLOT( slotAddDone( bool ) ) );
    kdDebug(1601) << "+ArkWidget::slotAddDone" << endl;
    archiveContent->setUpdatesEnabled(true);
    archiveContent->triggerUpdate();
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
    if (mpDownloadedList)
    {
        // is this necessary? Maybe risky. The tmp/ark.### directory
        // will be removed anyhow...
        KIO::del( *mpDownloadedList, false, false );
        delete mpDownloadedList;
        mpDownloadedList = NULL;
    }
    fixEnables();
    kdDebug(1601) << "-ArkWidget::slotAddDone" << endl;
}



KURL 
ArkWidget::toLocalFile( QString & str )
{
    KURL url = str;

    if(!url.isLocalFile())
    {
        if(!mpDownloadedList)
            mpDownloadedList = new QStringList();
        QString tempfile = m_settings->getTmpDir();
        tempfile += str.right(str.length() - str.findRev("/") - 1);
        if( !KIO::NetAccess::dircopy(url, tempfile, this) )
            return KURL();
        mpDownloadedList->append(tempfile);        // remember for deletion
        url = tempfile;
    }
    return url;
}

void 
ArkWidget::action_delete()
{
    // remove selected files and create a list to send to the archive
    // Warn the user if he/she/it tries to delete a directory entry in
    // a tar file - it actually deletes the contents of the directory
    // as well.

    kdDebug(1601) << "+ArkWidget::action_delete" << endl;

    if (archiveContent->isSelectionEmpty())
        return; // shouldn't happen - delete should have been disabled!

    bool bIsTar = (TAR_FORMAT == m_archType);
    bool bDeletingDir = false;
    QStringList list;
    FileLVI* flvi = (FileLVI*)archiveContent->firstChild();
    FileLVI* old_flvi;
    QStringList dirs;

    if (bIsTar)
    {
        // check if they're deleting a directory
        while (flvi)
        {
            if ( archiveContent->isSelected(flvi) )
            {
                old_flvi = flvi;
                flvi = (FileLVI*)flvi->itemBelow();
                QString strFile = old_flvi->fileName();
                list.append(strFile);
                QString strTemp = old_flvi->text(1);
                if (strTemp.left(1) == "d")
                {
                    bDeletingDir = true;
                    dirs.append(strFile);
                }
            }
            else
                flvi = (FileLVI*)flvi->itemBelow();
        }
        if (bDeletingDir)
        {
            int nRet = KMessageBox::warningContinueCancel(this, i18n("If you delete a folder in a Tar archive, all the files in that\nfolder will also be deleted. Are you sure you wish to proceed?"), i18n("Warning"), i18n("Continue"));
            if (nRet == KMessageBox::Cancel)
                return;
        }
    }

    bool bDoDelete = true;
    if (!bDeletingDir)
    {
        // ask for confirmation
        bDoDelete = KMessageBox::questionYesNo(this, i18n("Do you really want to delete the selected items?")) == KMessageBox::Yes;
    }
    if (!bDoDelete)
        return;

    // reset to the beginning to do the second sweep
    flvi = (FileLVI*)archiveContent->firstChild();
    while (flvi)
    {
        // if it's selected or, if it's a tar and we're deleting a directory
        // and the file is in that directory, delete the listview item.
        old_flvi = flvi;
        flvi = (FileLVI*)flvi->itemBelow();
        bool bDel = false;

        QString strFile = old_flvi->fileName();
        if (bIsTar && bDeletingDir)
        {
            for (QStringList::Iterator it = dirs.begin(); it != dirs.end(); ++it)
            {
                QRegExp re = "^" + *it;
                if (re.search(strFile) != -1)
                {
                    bDel = true;
                    break;
                }
            }
        }
        if (bDel || archiveContent->isSelected(old_flvi))
        {
            if (!bIsTar)
                list.append(strFile);
            delete old_flvi;
        }
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
    archiveContent->setUpdatesEnabled(true);
    archiveContent->triggerUpdate();
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
            SLOT( openWithSlotExtractDone() ) );

    showCurrentFile();
}

void 
ArkWidget::openWithSlotExtractDone()
{
    disconnect( arch, SIGNAL( sigExtract( bool ) ), this,
            SLOT( openWithSlotExtractDone() ) );

    KURL::List list;
    KURL url = m_strFileToView;
    list.append(url);
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
    if( archiveContent )
    {
        archiveContent->setUpdatesEnabled(true);
        fixEnables();
    }

}


void 
ArkWidget::prepareViewFiles( const QStringList & fileList )
{
    QString destTmpDirectory;
    destTmpDirectory = m_settings->getTmpDir();

    QStringList * list = new QStringList( fileList );
    arch->unarchFile( list, destTmpDirectory, true);
    delete list;
}

bool
ArkWidget::reportExtractFailures( const QString & _dest, QStringList *_list )
{
    // reports extract failures when Overwrite = False and the file
    // exists already in the destination directory.
    // If list is null, it means we are extracting all files.
    // Otherwise the list contains the files we are to extract.

    bool bRedoExtract = false;
    QString strFilename;

    QStringList list = *_list;
    QStringList filesExisting = existingFiles( _dest, list );

    int numFilesToReport = filesExisting.count();

    kdDebug(1601) << "There are " << numFilesToReport << " files to report existing already." << endl;

    // now report on the contents
    holdBusy();
    if (numFilesToReport == 1)
    {
        kdDebug(1601) << "One to report" << endl;
        strFilename = filesExisting.first();
        QString message =
            i18n("%1 will not be extracted because it will overwrite an existing file.\nGo back to Extract Dialog?").arg(strFilename);
        bRedoExtract = KMessageBox::questionYesNo(this, message) == KMessageBox::Yes;
    }
    else if (numFilesToReport != 0)
    {
        ExtractFailureDlg *fDlg = new ExtractFailureDlg( &filesExisting, this );
        bRedoExtract = !fDlg->exec();
    }
    resumeBusy();
    return bRedoExtract;
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
        // make the list
        FileListView *flw = fileList();
        FileLVI *flvi = (FileLVI*)flw->firstChild();
        while (flvi)
        {
            tmp = flvi->fileName();
            _list.append(tmp);
            flvi = (FileLVI*)flvi->itemBelow();
        }
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
    kdDebug(1601) << "+action_extract" << endl;
	 

	 KURL fileToExtract;
	 
    fileToExtract.setPath( arch->fileName() );
	 
	 kdDebug(1601) << "Archive to extract: " << fileToExtract.prettyURL() << endl;
	 
	 //before we start, make sure the archive is still there
    if (!KIO::NetAccess::exists( fileToExtract.prettyURL(), true, this ) )
    {
        KMessageBox::error(0, i18n("The archive to extract from no longer exists."));
        return false;
    }


    //if more than one entry in the archive is root level, suggest a path prefix
    QString prefix;
    int i = 0;

    for( FileLVI *pItem = (FileLVI *)archiveContent->firstChild();
         pItem;
         pItem = (FileLVI *)pItem->nextSibling() )
    {
      if( pItem->fileName().findRev( '/', -2 ) == -1 )
      {
        ++i;

        if( i > 1 )
        {
          prefix = QChar( '/' ) + guessName( fileToExtract.path() );
          kdDebug(1601) << "Archive requires extraction prefix: " << prefix << endl;
          break;
        }
      }
    }


    ExtractDlg *dlg = new ExtractDlg(m_settings, this, 0, prefix);

    // if they choose pattern, we have to tell arkwidget to select
    // those files... once we're in the dialog code it's too late.
    connect( dlg, SIGNAL( pattern( const QString & ) ), this, SLOT( selectByPattern( const QString & ) ) );
    bool bRedoExtract = false;

    if (m_nNumSelectedFiles == 0)
    {
        dlg->disableSelectedFilesOption();
    }
    if (archiveContent->currentItem() == NULL)
    {
        dlg->disableCurrentFileOption();
    }

    // list of files to be extracted
    m_extractList = new QStringList;
    if ( dlg->exec() )
    {
        int extractOp = dlg->extractOp();
        kdDebug(1601) << "Extract op: " << extractOp << endl;

        //m_extractURL will always be the location the user chose to
        //m_extract to, whether local or remote
        m_extractURL = dlg->extractDir();

        //extractDir will either be the real, local extract dir,
        //or in case of a extract to remote location, a local tmp dir
        QString extractDir;

        if ( !m_extractURL.isLocalFile() )
        {
            m_extractRemoteTmpDir = new KTempDir( m_settings->getTmpDir() + "extremote" );
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
        bool bOvwrt = m_settings->getExtractOverwrite();

        switch(extractOp)
        {
        case ExtractDlg::All:
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
                    arch->unarchFile(0, extractDir);
                }
            }
            break;
        case ExtractDlg::Pattern:
        case ExtractDlg::Selected:
        case ExtractDlg::Current:
            {
                int nTotalSize = 0;
                if (extractOp != ExtractDlg::Current )
                {
                    // make a list to send to unarchFile
                    FileListView *flw = fileList();
                    FileLVI *flvi = (FileLVI*)flw->firstChild();
                    while (flvi)
                    {
                        if ( flw->isSelected(flvi) )
                        {
                            kdDebug(1601) << "unarching " << flvi->fileName() << endl;
                            QCString tmp = QFile::encodeName(flvi->fileName());
                            m_extractList->append(tmp);
                            nTotalSize += flvi->fileSize();
                        }
                        flvi = (FileLVI*)flvi->itemBelow();
                    }
                }
                else
                {
                    FileLVI *pItem = archiveContent->currentItem();
                    if (pItem == 0)
                    {
                        kdDebug(1601) << "Can't seem to figure out which is current!" << endl;
                        return true;
                    }
                    QString tmp = pItem->fileName();  // no text(0)
                    nTotalSize += pItem->fileSize();
                    m_extractList->append( QFile::encodeName(tmp) );
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
                        arch->unarchFile(m_extractList, extractDir); // extract selected files
                    }
                }
                break;
            }
        default:
            Q_ASSERT(0);
            // never happens
            break;
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
    if( archiveContent )
    {
        archiveContent->setUpdatesEnabled(true);
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
        m_strFileToView = m_strFileToView.right(m_strFileToView.length() - 5 );
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
    arch->addFile( &list );

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
             SLOT( viewSlotExtractDone() ) );
    busy( i18n( "Extracting file to view" ) );
    showCurrentFile();
}

void 
ArkWidget::viewSlotExtractDone()
{
    chmod( QFile::encodeName( m_strFileToView ), 0400 );
    QString mimetype = KMimeType::findByURL( m_strFileToView )->name();
    bool view = true;
    
    if ( KRun::isExecutable( mimetype ) )
    {
    	QString text = i18n( "The file you're trying to view may be an executable. Running untrusted executables may compromisse your system's security.\nAre you sure you want to view that file?" );
        view = ( KMessageBox::warningYesNo( this, text ) == KMessageBox::Yes );
    }

    if ( view )
    	KRun::runURL( m_strFileToView, mimetype, true );

    disconnect( arch, SIGNAL( sigExtract( bool ) ), this,
                SLOT( viewSlotExtractDone( ) ) );
    // avoid race condition, don't do updates if application is exiting
    if( archiveContent )
    {
        archiveContent->setUpdatesEnabled(true);
        fixEnables();
    }
    ready();
}


void 
ArkWidget::showCurrentFile()
{
    FileLVI *pItem = archiveContent->currentItem();

    if ( pItem == NULL )
        return;

    QString name = pItem->fileName(); // no text(0)

    QString fullname;
    fullname = "file:";
    fullname += m_settings->getTmpDir();
    fullname += "/";
    fullname += name;

    kdDebug(1601) << "File to be viewed: " << fullname << endl;

    QStringList extractList;
    extractList.append(name);

    m_strFileToView = fullname;
    if (ArkUtils::diskHasSpace( m_settings->getTmpDir(), pItem->fileSize() ) )
    {
        disableAll();
        prepareViewFiles( extractList );
    }
}

// Options menu //////////////////////////////////////////////////////

void
ArkWidget::options_dirs()
{
    GeneralOptDlg *dd = new GeneralOptDlg( m_settings, this );
    dd->exec();
    delete dd;
}


void
ArkWidget::options_saveNow()
{
    m_settings->writeConfigurationNow();
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
        archiveContent->setCurrentItem(pItem);
        archiveContent->setSelected(pItem, true);
        emit signalFilePopup( pPoint );
    }
    else // clicked anywhere else but the name column
    {
        emit signalArchivePopup( pPoint );
    }
}


void 
ArkWidget::viewFile() // slot
// show contents when double click
{
	emit action_view();
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
    kdDebug( 1601 )<< "update Status Selection" << endl;
    m_nNumSelectedFiles = 0;
    m_nSizeOfSelectedFiles = 0;

    if (archiveContent)
    {
        FileLVI * flvi = (FileLVI*)archiveContent->firstChild();
        while (flvi)
        {
            if (flvi->isSelected())
            {
                ++m_nNumSelectedFiles;
                m_nSizeOfSelectedFiles += flvi->fileSize();
            }
            flvi = (FileLVI*)flvi->itemBelow();
        }
    }
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


void 
ArkWidget::selectByPattern(const QString & _pattern) // slot
{
    // select all the files that match the pattern

    FileLVI * flvi = (FileLVI*)archiveContent->firstChild();
    QRegExp *glob = new QRegExp(_pattern, true, true); // file globber

    archiveContent->clearSelection();
    while (flvi)
    {
        if (glob->search(flvi->fileName()) != -1)
            archiveContent->setSelected(flvi, true);
        flvi = (FileLVI*)flvi->itemBelow();
    }

    delete glob;
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
                       i18n("Add"), i18n("Open"));
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
            int nRet = KMessageBox::warningYesNo(this, str);
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
        url.setPath( m_settings->getTmpDir() + '/' + *it );
        list.append( url );
    }

    KURLDrag *drg = new KURLDrag(list, archiveContent->viewport(), "Ark Archive Drag" );
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
	kdDebug() << "ArkWidget::createFileListView" << endl;
	//delete archiveContent;
	if ( !archiveContent )
	{
		archiveContent = new FileListView(this, this);
		archiveContent->setMultiSelection(true);
		archiveContent->show();
		connect( archiveContent, SIGNAL( selectionChanged()), this, SLOT( slotSelectionChanged() ) );
		connect( archiveContent, SIGNAL( rightButtonPressed(QListViewItem *, const QPoint &, int)),
				this, SLOT(doPopup(QListViewItem *, const QPoint &, int)));
		connect( archiveContent, SIGNAL( startDragRequest( const QStringList & ) ),
				this, SLOT( startDrag( const QStringList & ) ) );
		connect( archiveContent, SIGNAL( doubleClicked(QListViewItem *, const QPoint &, int ) ), 
				this, SLOT( viewFile() ) );
    }
    archiveContent->clear();
}


Arch * ArkWidget::getNewArchive( const QString & _fileName )
{
    Arch * newArch = 0;

    QString type = KMimeType::findByURL( _fileName )->name();
    ArchType archtype = ArchiveFormatInfo::self()->archTypeForMimeType(type);
    kdDebug( 1601 ) << "archtype is recognised as: " << archtype << endl;
    if(0 == (newArch = Arch::archFactory(archtype, m_settings, this,
                                         _fileName)))
    {
        KMessageBox::error(this, i18n("Unknown archive format or corrupted archive") );
        return NULL;
    }

    if (!newArch->utilityIsAvailable())
    {
        KMessageBox::error(this, i18n("The utility %1 is not in your PATH.\nPlease install it or contact your system administrator.").arg(newArch->getUtility()));
        return NULL;
    }

    m_archType = archtype;
    archiveContent->setUpdatesEnabled(true);
    return newArch;
}

//////////////////////////////////////////////////////////////////////
////////////////////// createArchive /////////////////////////////////
//////////////////////////////////////////////////////////////////////


void 
ArkWidget::createArchive( const QString & _filename )
{
    Arch * newArch = getNewArchive( _filename );
    if ( !newArch )
        return;

    busy( i18n( "Creating archive..." ) );
    connect( newArch, SIGNAL(sigCreate(Arch *, bool, const QString &, int) ),
             this, SLOT(slotCreate(Arch *, bool, const QString &, int) ) );

    newArch->create();
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
    if( 0 == ( newArch = Arch::archFactory( archtype, m_settings, this,
                                            _filename, m_openAsMimeType) ) )
    {
        emit setWindowCaption( QString::null );
        emit removeRecentURL( m_realURL );
        KMessageBox::error( this, i18n("Unknown archive format or corrupted archive") );
        return;
    }

    if (!newArch->utilityIsAvailable())
    {
        KMessageBox::error(this, i18n("The utility %1 is not in your PATH.\nPlease install it or contact your system administrator.").arg(newArch->getUtility()));
        return;
    }

    m_archType = archtype;

    connect( newArch, SIGNAL(sigOpen(Arch *, bool, const QString &, int)),
             this, SLOT(slotOpen(Arch *, bool, const QString &,int)) );

    disableAll();

    busy( i18n( "Opening the archive..." ) );
    archiveContent->setUpdatesEnabled( false );
    arch = newArch;
    newArch->open();
}

void
ArkWidget::slotOpen( Arch * /* _newarch */, bool _success, const QString & _filename, int )
{
    ready();
    kdDebug(1601) << "+ArkWidget::slotOpen" << endl;
    archiveContent->setUpdatesEnabled(true);
    archiveContent->triggerUpdate();

    if ( _success )
    {
        QFileInfo fi( _filename );
        QString path = fi.dirPath( true );
        m_settings->setLastOpenDir( path );

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
    kdDebug(1601) << "-ArkWidget::slotOpen" << endl;
}

#include "arkwidget.moc"
