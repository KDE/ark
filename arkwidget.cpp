/*
 Emacs: -*- mode: c++; c-basic-offset: 4; -*-

 ark -- archiver for the KDE project

 Copyright (C)

 2003: Georg Robbers <Georg.Robbers@urz.uni-hd.de>
 2002: Helio Chissini de Castro <helio@conectiva.com.br>
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
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

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
#include <kstandarddirs.h>
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
        m_extractOnly(false),
        m_extractRemote(false), m_openAsMimeType(QString::null),
        m_pTempAddList(NULL), mpDownloadedList(NULL)
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

    kdDebug(1601) << "-ArkWidget::~ArkWidget" << endl;
}


////////////////////////////////////////////////////////////////////
///////////////////////// updateStatusTotals ///////////////////////
////////////////////////////////////////////////////////////////////

void ArkWidget::updateStatusTotals()
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
    //  kdDebug(1601) << "We have " << m_nNumFiles << " elements\n" << endl;

    QString strInfo = i18n("%n file  %1", "%n files  %1", m_nNumFiles).arg(KIO::convertSize(m_nSizeOfFiles));
    emit setStatusBarText(strInfo);
}

//////////////////////////////////////////////////////////////////////
////////////////////// file_save_as //////////////////////////////////
//////////////////////////////////////////////////////////////////////

KURL ArkWidget::getSaveAsFileName()
{
    QStringList list;
    if ( m_openAsMimeType.isNull() )
        list = KMimeType::findByPath( m_strArchName )->patterns();
    else
        list = KMimeType::mimeType( m_openAsMimeType )->patterns();

    KURL u;
    do
    {
        u = getCreateFilename( i18n( "Save Archive As" ), list.join( "\n" ), list.first().remove( '*' ) );
        if (  u.isEmpty() )
            return u;
        // we have to make sure the user doesn't think this is
        // an opportunity to convert .tgz to .zip...
        if( allowedArchiveName( u ) )
            break;
        KMessageBox::error( this, i18n( "Please save your archive in the same format as the original.\nHint: Use one of the suggested extensions." ) );
    }
    while ( true );
    return u;
}

bool ArkWidget::file_save_as( const KURL & u )
{
    // synchronous, because saveFile (in the part) has to return after the file
    // has been saved
    // upload, so that overwrite is true
    return KIO::NetAccess::upload( m_strArchName, u );
    /*
    KIO::Job * job = KIO::copy(src, u);
      connect( job, SIGNAL( result( KIO::Job * ) ), this,
               SLOT( slotSaveAsDone( KIO::Job * ) ) );
    */
}

bool ArkWidget::allowedArchiveName( const KURL & u )
{
    if (u.isEmpty())
        return false;

    enum ArchType archtype = ArchiveFormatInfo::self()->archTypeForURL( m_url );
    if ( !m_openAsMimeType.isNull() )
        archtype = ArchiveFormatInfo::self()->archTypeForMimeType( m_openAsMimeType );

    QString strFile = u.path();
    ArchType newArchType = ArchiveFormatInfo::self()->archTypeForURL( u );

    if (newArchType == archtype)
        return true;
    // these types don't mind having no extension. Zip will add one, ulp!
    if (newArchType == UNKNOWN_FORMAT && !strFile.contains('.')
            && (archtype == RAR_FORMAT || archtype == LHA_FORMAT ||
                archtype == AA_FORMAT))
        return true;
    return false;
}

void ArkWidget::slotSaveAsDone(KIO::Job * job)
{
    if (job->error())
        job->showErrorDialog();
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
        return;

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
        emit removeRecentURL(strFile);
        return;
    }
    else if ( !fileInfo.isReadable() )
    {
        KMessageBox::error(this, i18n("You do not have permission to access that archive.") );
        emit removeRecentURL(strFile);
        return;
    }

    // see if the user is just opening the same file that's already
    // open (erm...)

    if (strFile == m_strArchName && m_bIsArchiveOpen)
    {
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

KURL ArkWidget::getCreateFilename(const QString & _caption,
                                  const QString & _filter,
                                  const QString & _extension,
                                  bool allowCompressed)
{
    int choice=0;
    bool skip = false;
    QString strFile;
    KURL url;

    while (true)
        // keep asking for filenames as long as the user doesn't want to
        // overwrite existing ones; break if they agree to overwrite
        // or if the file doesn't already exist. Return if they cancel.
        // Also check for proper extensions.
    {
        if (!skip)
        {
            url = KFileDialog::getSaveURL(QString::null, _filter,
                                                    0, _caption);
            strFile = url.path();
        }
        skip = false;
        if (strFile.isEmpty())
            return QString::null;

        //the user chose to save as the current archive
        //or wanted to create a new one with the same name
        //no need to do anything
        if (strFile == m_strArchName && m_bIsArchiveOpen)
            return QString::null;

        kdDebug(1601) << "Trying to create an archive named " <<
        strFile << endl;
        if( QFile::exists( strFile ) ) // already exists!
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
        if ( !ArkUtils::haveDirPermissions( strFile ) )
            return QString::null;

        if ( !allowCompressed &&
                ArchiveFormatInfo::self()->archTypeByExtension( url.path() ) == COMPRESSED_FORMAT)
        {
            KMessageBox::information(0, i18n("You need to create an archive, not a new\ncompressed file. Please try again."));
            continue;
        }

        // if we made it here, it's a go.
        if ((m_strArchName.contains('.') && !strFile.contains('.')) ||
                (m_strArchName.isNull() && !strFile.contains('.')))
        {
            // if the filename has no dot in it, ask to append extension
            QString ext = _extension;
            if (ext.isNull())
                ext = ".zip";

            int nRet = KMessageBox::warningYesNo(0, i18n("Your file is missing an extension to indicate the archive type.\nIs it OK to create a file of the default type (%1)?").arg(ext), i18n("Error"));
            if (nRet == KMessageBox::Yes)
            {
                strFile += ext;
                url = strFile;
                skip = true; // skip the getSaveUrl part
                continue; // gotta check if it exists again
            }
            else // no? well choose a different filename then.
            {
                continue;
            }
        }
        else
            break;
    } // end of while loop
    return url;
}

void ArkWidget::file_new()
{
    QString strFile;
    KURL url = getCreateFilename(i18n("Create New Archive"),
                                 ArchiveFormatInfo::self()->filter());
    strFile = url.path();
    if (!strFile.isEmpty())
    {
        m_settings->clearShellOutput();
        file_close();
        createArchive( strFile );
    }
}

void ArkWidget::extractOnlyOpenDone()
{
    int oldMode = m_settings->getExtractDirMode();
    QString oldFixedExtractDir = m_settings->getFixedExtractDir();
    m_settings->setExtractDirCfg(m_url.upURL().path(), ArkSettings::FIXED_EXTRACT_DIR);
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
    kdDebug(1601) << "+ArkWidget::slotExtractDone" << endl;
    QApplication::restoreOverrideCursor();

    if(m_extractList != 0)
        delete m_extractList;
    m_extractList = 0;

    if( archiveContent ) // avoid race condition, don't do updates if application is exiting
    {
        archiveContent->setUpdatesEnabled(true);
        fixEnables();
    }

    if ( m_extractRemote )
        extractRemoteInitiateMoving();

    if(m_extractOnly)
    {
        emit request_file_quit();  // Close ark window if we are doing an Extract Here, the toplevel window connects to this
    }

    kdDebug(1601) << "-ArkWidget::slotExtractDone" << endl;
}

void ArkWidget::extractRemoteInitiateMoving()
{
    KURL srcDirURL( m_settings->getTmpDir() + "extrtmp/" );
    KURL src;
    QString srcDir( m_settings->getTmpDir() + "extrtmp/" );
    QDir dir( srcDir );
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

    KIO::CopyJob *job = KIO::copy( srcList, m_extractURL );
    connect( job, SIGNAL(result(KIO::Job*)),
            this, SLOT(slotExtractRemoteDone(KIO::Job*)) );

    m_extractRemote = false;
}

void ArkWidget::slotExtractRemoteDone(KIO::Job *job)
{
    QDir dir( m_settings->getTmpDir() + "extrtmp/" );
    dir.rmdir( dir.absPath()  );

    if ( job->error() )
        job->showErrorDialog();
}


void ArkWidget::disableAll() // private
{
    kdDebug(1601) << "+ArkWidget::disableAll" << endl;
    emit disableAllActions();
    archiveContent->setUpdatesEnabled(true);
    QApplication::setOverrideCursor( waitCursor );
    kdDebug(1601) << "-ArkWidget::disableAll" << endl;
}

void ArkWidget::fixEnables() // private
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

void ArkWidget::edit_select()
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

void ArkWidget::edit_selectAll()
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

void ArkWidget::edit_deselectAll()
{
    archiveContent->clearSelection();
    updateStatusSelection();
}

void ArkWidget::edit_invertSel()
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

void ArkWidget::edit_view_last_shell_output()
{
    viewShellOutput();
}

KURL ArkWidget::askToCreateRealArchive()
{
    // ask user whether to create a real archive from a compressed file
    // returns filename if so
    KURL url;
    int choice =
        KMessageBox::warningYesNo(0, i18n("You are currently working with a simple compressed file.\nWould you like to make it into an archive so that it can contain multiple files?\nIf so, you must choose a name for your new archive."), i18n("Warning"));
    if (choice == KMessageBox::Yes)
    {
        url = getCreateFilename( i18n("Create New Archive"),
                                 ArchiveFormatInfo::self()->filter(),
                                 QString::null, false );
    }
    else
        url.setPath( QString::null );
    return url;
}

void ArkWidget::createRealArchive( const QString & strFilename, const QStringList & filesToAdd )
{
    Arch * newArch = getNewArchive( strFilename );
    if ( !newArch )
        return;
    if ( !filesToAdd.isEmpty() )
        m_pTempAddList = new QStringList( filesToAdd );
    // kdDebug( 1601 ) << " ---- " << filesToAdd << endl;
    m_compressedFile = static_cast< CompressedFile * >( arch )->tempFileName();
    // kdDebug(1601) << "The compressed file is " << m_compressedFile << endl;
    connect( newArch, SIGNAL( sigCreate( Arch *, bool, const QString &, int ) ),
             this, SLOT( createRealArchiveSlotCreate( Arch *, bool,
             const QString &, int ) ) );
    file_close();
    newArch->create();
}

void ArkWidget::createRealArchiveSlotCreate( Arch * newArch, bool success,
                                             const QString & fileName, int nbr )
{
    kdDebug( 1601 ) << "+createRealArchiveSlotCreate" << endl;
    slotCreate( newArch, success, fileName, nbr );
    kdDebug( 1601 ) << "slotCreate called, success is: " << success << endl;
    if ( !success )
        return;

    QStringList listForCompressedFile;
    listForCompressedFile.append(m_compressedFile);
    disableAll();

    connect( newArch, SIGNAL( sigAdd( bool ) ), this,
                      SLOT( createRealArchiveSlotAddDone( bool ) ) );

    newArch->addFile(&listForCompressedFile);
}

void ArkWidget::createRealArchiveSlotAddDone( bool success )
{
    kdDebug( 1601 ) << "createRealArchiveSlotAddDone+, success:" << success << endl;
    disconnect( arch, SIGNAL( sigAdd( bool ) ), this,
                      SLOT( createRealArchiveSlotAddDone( bool ) ) );
    connect( arch, SIGNAL( sigAdd( bool ) ), this,
                   SLOT( createRealArchiveSlotAddFilesDone( bool ) ) );

    if ( !success )
        return;

    QApplication::restoreOverrideCursor();
    if ( m_pTempAddList == NULL )
    {
        // now get the files to be added
        // we don't know which files to add yet
        action_add();
    }
    else
    {
        // files were dropped in
        addFile( m_pTempAddList );
    }
}

void ArkWidget::createRealArchiveSlotAddFilesDone( bool success )
{
    kdDebug( 1601 ) << "createRealArchiveSlotAddFilesDone+, success:" << success << endl;
    disconnect( arch, SIGNAL( sigAdd( bool ) ), this,
                      SLOT( createRealArchiveSlotAddFilesDone( bool ) ) );
    delete m_pTempAddList;
    m_pTempAddList = NULL;
}




// Action menu /////////////////////////////////////////////////////////

void ArkWidget::action_add()
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

void ArkWidget::addFile(QStringList *list)
{
    if ( !ArkUtils::diskHasSpace( m_strArchName, ArkUtils::getSizes( list ) ) )
        return;

    disableAll();
    // if they are URLs, we have to download them, replace the URLs
    // with filenames, and remember to delete the temporaries later.
    for (QStringList::Iterator it = list->begin(); it != list->end(); ++it)
    {
        QString str = *it;
        kdDebug(1601) << "Want to add " << str<< endl;
        KURL url( toLocalFile(str) );
        *it = url.prettyURL();

    }

    connect( arch, SIGNAL( sigAdd( bool ) ), this, SLOT( slotAddDone( bool ) ) );
    arch->addFile(list);
}

void ArkWidget::action_add_dir()
{
    KURL u = KDirSelectDialog::selectDirectory( m_settings->getAddDir(),
                                                    false, this,
                                                    i18n("Select Directory to Add"));

    QString dir = KURL::decode_string( u.url(-1) );
    if ( !dir.isEmpty() )
    {
        disableAll();
        u = toLocalFile(dir);
        connect( arch, SIGNAL( sigAdd( bool ) ), this, SLOT( slotAddDone( bool ) ) );
        arch->addDir( u.prettyURL() );
    }

}

void ArkWidget::slotAddDone(bool _bSuccess)
{
    disconnect( arch, SIGNAL( sigAdd( bool ) ), this, SLOT( slotAddDone( bool ) ) );
    kdDebug(1601) << "+ArkWidget::slotAddDone" << endl;
    archiveContent->setUpdatesEnabled(true);
    archiveContent->triggerUpdate();

    if (_bSuccess)
    {
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
    QApplication::restoreOverrideCursor();
    kdDebug(1601) << "-ArkWidget::slotAddDone" << endl;
}



KURL ArkWidget::toLocalFile( QString & str)
{
    KURL url = str;

    if(!url.isLocalFile())
    {
        if(!mpDownloadedList)
            mpDownloadedList = new QStringList();
        QString tempfile = m_settings->getTmpDir();
        tempfile += str.right(str.length() - str.findRev("/") - 1);
        if( !KIO::NetAccess::dircopy(url, tempfile) )
            return KURL();
        mpDownloadedList->append(tempfile);        // remember for deletion
        url = tempfile;
    }
    return url;
}

void ArkWidget::action_delete()
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
            int nRet = KMessageBox::warningContinueCancel(this, i18n("If you delete a directory in a Tar archive, all the files in that\ndirectory will also be deleted. Are you sure you wish to proceed?"), i18n("Warning"), i18n("Continue"));
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
    connect( arch, SIGNAL( sigDelete( bool ) ), this, SLOT( slotDeleteDone( bool ) ) );
    arch->remove(&list);
    kdDebug(1601) << "-ArkWidget::action_delete" << endl;
}

void ArkWidget::slotDeleteDone(bool _bSuccess)
{
    disconnect( arch, SIGNAL( sigDelete( bool ) ), this, SLOT( slotDeleteDone( bool ) ) );
    kdDebug(1601) << "+ArkWidget::slotDeleteDone" << endl;
    archiveContent->setUpdatesEnabled(true);
    archiveContent->triggerUpdate();
    if (_bSuccess)
    {
        updateStatusTotals();
        updateStatusSelection();
    }
    // disable the select all and extract options if there are no files left
    fixEnables();
    QApplication::restoreOverrideCursor();
    kdDebug(1601) << "-ArkWidget::slotDeleteDone" << endl;

}



void ArkWidget::slotOpenWith()
{
    connect( arch, SIGNAL( sigExtract( bool ) ), this,
            SLOT( openWithSlotExtractDone() ) );

    showCurrentFile();
}

void ArkWidget::openWithSlotExtractDone()
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


void ArkWidget::prepareViewFiles( const QStringList & fileList )
{
    QString destTmpDirectory;
    destTmpDirectory = m_settings->getTmpDir();

    QDir dir( destTmpDirectory );

    //shouldn't happen, already created in the ctor
    if( ! dir.exists( destTmpDirectory ) )
    {
        kdDebug(1601) << "Creating tmp view dir: " << destTmpDirectory << endl;
        dir.mkdir( destTmpDirectory );
    }
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

    QString strFilename, tmp;
    bool bRedoExtract = false;

    QApplication::restoreOverrideCursor();

    Q_ASSERT(_list != NULL);
    QString strDestDir = _dest;

    // make sure the destination directory has a / at the end.
    if (strDestDir.at(0) != '/')
    {
        strDestDir += '/';
    }

    if (_list->isEmpty())
    {
        // make the list
        FileListView *flw = fileList();
        FileLVI *flvi = (FileLVI*)flw->firstChild();
        while (flvi)
        {
            tmp = flvi->fileName();
            _list->append(tmp);
            flvi = (FileLVI*)flvi->itemBelow();
        }
    }

    QStringList existingFiles;
    // now the list contains all the names we must verify.
    for (QStringList::Iterator it = _list->begin(); it != _list->end(); ++it)
    {
        strFilename = *it;
        QString strFullName = strDestDir + strFilename;
        if ( QFile::exists( strFullName ) )
        {
            existingFiles.append( strFilename );
        }
    }

    int numFilesToReport = existingFiles.count();

    kdDebug(1601) << "There are " << numFilesToReport << " files to report existing already." << endl;

    // now report on the contents
    if (numFilesToReport == 1)
    {
        kdDebug(1601) << "One to report" << endl;
        strFilename = *(existingFiles.at(0));
        QString message =
            i18n("%1 will not be extracted because it will overwrite an existing file.\nGo back to Extract Dialog?").arg(strFilename);
        bRedoExtract =	KMessageBox::questionYesNo(this, message) == KMessageBox::Yes;
    }
    else if (numFilesToReport != 0)
    {
        ExtractFailureDlg *fDlg = new ExtractFailureDlg( &existingFiles, this );
        bRedoExtract = !fDlg->exec();
    }
    return bRedoExtract;
}

bool
ArkWidget::action_extract()
{
    kdDebug(1601) << "+action_extract" << endl;
    //before we start, make sure the archive is still there
    if (!KIO::NetAccess::exists(KURL(arch->fileName())))
    {
        KMessageBox::error(0, i18n("The archive to extract from no longer exists."));
        // emit request_file_quit();
        return false;
    }

    ExtractDlg *dlg = new ExtractDlg(m_settings);

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
            extractDir = m_settings->getTmpDir() + "extrtmp/";
            m_extractRemote = true;
            //make sure it's empty since all of it's contents
            //will be copied to the remote extract location
            KIO::NetAccess::del( extractDir );
            if ( !KIO::NetAccess::mkdir( extractDir ) )
            {
                kdWarning(1601) << "Unable to create " << extractDir << endl;
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
        return false;
    }

    // user might want to change some options or the selection...
    if (bRedoExtract)
    {
        return action_extract();
    }

    return true;
}

void ArkWidget::action_edit()
{
    // begin an edit. This is like a view, but once the process exits,
    // the file is put back into the archive. If the user tries to quit or
    // close the archive, there will be a warning that any changes to the
    // files open under "Edit" will be lost unless the archive remains open.
    // [hmm, does that really make sense? I'll leave it for now.]

    connect( arch, SIGNAL( sigExtract( bool ) ), this,
                        SLOT( editSlotExtractDone() ) );
    showCurrentFile();
}

void ArkWidget::editSlotExtractDone()
{
    disconnect( arch, SIGNAL( sigExtract( bool ) ),
                this, SLOT( editSlotExtractDone() ) );

    editStart();

    // avoid race condition, don't do updates if application is exiting
    if( archiveContent )
    {
        archiveContent->setUpdatesEnabled(true);
        fixEnables();
    }
}
void ArkWidget::editStart()
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

void ArkWidget::slotEditFinished(KProcess *kp)
{
    connect( arch, SIGNAL( sigAdd( bool ) ), this, SLOT( editSlotAddDone( bool ) ) );
    kdDebug(1601) << "+ArkWidget::slotEditFinished" << endl;
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

    arch->addFile( &list );

    kdDebug(1601) << "-ArkWidget::slotEditFinished" << endl;
}

void ArkWidget::editSlotAddDone( bool success )
{
    disconnect( arch, SIGNAL( sigAdd( bool ) ), this, SLOT( editSlotAddDone( bool ) ) );
    slotAddDone( success );
}

void ArkWidget::action_view()
{
    connect( arch, SIGNAL( sigExtract( bool ) ), this,
             SLOT( viewSlotExtractDone() ) );
    showCurrentFile();
}

void ArkWidget::viewSlotExtractDone()
{
    QApplication::restoreOverrideCursor();
    m_pKRunPtr = new KRun( m_strFileToView );
    disconnect( arch, SIGNAL( sigExtract( bool ) ), this,
                SLOT( viewSlotExtractDone( ) ) );
    // avoid race condition, don't do updates if application is exiting
    if( archiveContent )
    {
        archiveContent->setUpdatesEnabled(true);
        fixEnables();
    }
}


void ArkWidget::showCurrentFile()
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
ArkWidget::doPopup( QListViewItem *pItem, const QPoint &pPoint, int nCol ) // slot
// do the right-click popup menus
{
    kdDebug(1601) << "+ArkWidget::doPopup" << endl;
    if (nCol == 0)
    {
        archiveContent->setCurrentItem(pItem);
        archiveContent->setSelected(pItem, true);
        emit signalFilePopup( pPoint );
    }
    else // clicked anywhere else but the name column
    {
        emit signalArchivePopup( pPoint ); //signalArchivePopup( pPoint );
    }
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

void ArkWidget::updateStatusSelection()
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


void ArkWidget::selectByPattern(const QString & _pattern) // slot
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

#if 0 // not sure I need this
void ArkWidget::dragEnterEvent(QDragEnterEvent* event)
{
    event->accept(QUriDrag::canDecode(event));
}

#endif

void ArkWidget::dragMoveEvent(QDragMoveEvent *e)
{
    if (QUriDrag::canDecode(e) && !m_bDropSourceIsSelf)
    {
        e->accept();
    }
}


void ArkWidget::dropEvent(QDropEvent* e)
{
    kdDebug( 1601 ) << "+ArkWidget::dropEvent" << endl;

    QStringList list;

    if ( QUriDrag::decodeToUnicodeUris( e, list ) )
    {
        dropAction( list );
    }

    kdDebug(1601) << "-dropEvent" << endl;
}

//////////////////////////////////////////////////////////////////////
///////////////////////// dropAction /////////////////////////////////
//////////////////////////////////////////////////////////////////////

void ArkWidget::dropAction( QStringList  & list )
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

        // ###FIXME: needs to emit something like openURLRequest for the
        // Shell, so that dropping of remote files works...needs
        // a BrowserExtension
        file_open(url);
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

void ArkWidget::startDrag( const QStringList & fileList )
{
    mDragFiles = fileList;
    connect( arch, SIGNAL( sigExtract( bool ) ), this, SLOT( startDragSlotExtractDone( bool ) ) );
    prepareViewFiles( fileList );
}

void ArkWidget::startDragSlotExtractDone( bool )
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

//////////////////////////////////////////////////////////////////////
///////////////////////// showFavorite ///////////////////////////////
//////////////////////////////////////////////////////////////////////

//?Question: not used, what is this supposed to do? -> not ported yet

void ArkWidget::showFavorite()
{
/*
      const QFileInfoList *flist;
      QDir *fav;

      file_close();
      createFileListView();

      archiveContent->addColumn( i18n(" File ") );
      archiveContent->addColumn( i18n(" Size ") );
      archiveContent->setColumnAlignment(1, AlignRight);
      archiveContent->setMultiSelection( false );

      fav = new QDir( m_settings->getFavoriteDir() );
      if ( !fav->exists() )
        {
          KMessageBox::error( this, i18n("Archive directory does not exist."));
          return;
        }
      flist = fav->entryInfoList();
      QFileInfoListIterator flisti( *flist );
      ++flisti; // Skip . directory

      if ( (flisti.current())->fileName() == ".." )
        {
          FileLVI *flvi = new FileLVI(archiveContent);
          flvi->setText(0, "..");
          archiveContent->insertItem(flvi);
          ++flisti;
        }

      QString size;
      bool isDirectory;
      for( uint i=0; i < flist->count()-2; i++ )
        {
          QString name( (flisti.current())->fileName() );
          isDirectory = (flisti.current())->isDir();
          if ( (Arch::getArchType(name)!=-1) || (isDirectory) )
            {
              FileLVI *flvi = new FileLVI(archiveContent);
              flvi->setText(0, name);
              if (!isDirectory)
                {
                  size = KGlobal::locale()->formatNumber(flisti.current()->size(), 0);
                  flvi->setText(1, size);
                  archiveContent->insertItem(flvi);
                }
            }
          ++flisti;
        }
      archiveContent->setColumnWidth(0, archiveContent->columnWidth(0) + 10 );

      emit setWindowCaption( m_settings->getFavoriteDir() );

      delete fav;

      //    writeStatusMsg( i18n( "Archive Directory") );
*/
}

/**
 * Writes a message in the status bar.
 * This message is visible during 5 seconds.
 */
#if 0
void ArkWidget::writeStatusMsg(const QString text)
{
    statusBarTimer->stop();
    statusBar()->changeItem(text, 0);
    statusBarTimer->start(5000,true);
}
#endif

#if 0
void ArkWidget::clearStatusBar()
{
    statusBar()->changeItem(QString::null,0);
}
#endif


void ArkWidget::arkWarning(const QString& msg)
{
    KMessageBox::information(this, msg);
}

#if 0
void ArkWidget::slotStatusBarTimeout()

clearStatusBar();
}
#endif


void ArkWidget::createFileListView()
{
    kdDebug() << "ArkWidget::createFileListView" << endl;
    //delete archiveContent;
    if ( !archiveContent )
    {
        archiveContent = new FileListView(this, this);
        archiveContent->setMultiSelection(true);
        archiveContent->show();
        connect( archiveContent, SIGNAL( selectionChanged()),
                 this, SLOT( slotSelectionChanged() ) );
        connect(archiveContent,
                SIGNAL(rightButtonPressed(QListViewItem *,
                                          const QPoint &, int)),
                this, SLOT(doPopup(QListViewItem *,
                                   const QPoint &, int)));
        connect( archiveContent, SIGNAL( startDragRequest( const QStringList & ) ),
                 this, SLOT( startDrag( const QStringList & ) ) );
    }
    archiveContent->clear();
}


Arch * ArkWidget::getNewArchive( const QString & _fileName )
{
    Arch * newArch = 0;

    ArchType archtype = ArchiveFormatInfo::self()->archTypeByExtension(_fileName);
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


void ArkWidget::createArchive( const QString & _filename )
{
    Arch * newArch = getNewArchive( _filename );
    if ( !newArch )
        return;

    QApplication::setOverrideCursor( waitCursor );

    connect( newArch, SIGNAL(sigCreate(Arch *, bool, const QString &, int)),
             this, SLOT(slotCreate(Arch *, bool, const QString &, int)) );

    newArch->create();
}

void ArkWidget::slotCreate(Arch * _newarch, bool _success,
                           const QString & _filename, int)
{
    disconnect( _newarch, SIGNAL( sigCreate( Arch *, bool, const QString &, int ) ),
                0,0 );

    if ( _success )
    {
        //file_close(); already called in ArkWidget::file_new()
        m_strArchName = _filename;
        emit setWindowCaption( _filename );
        emit addRecentURL(_filename);
        createFileListView();
        m_bIsArchiveOpen = true;
        arch = _newarch;
        m_bIsSimpleCompressedFile =
            (m_archType == COMPRESSED_FORMAT);
        fixEnables();
        QApplication::restoreOverrideCursor();
    }
    else
    {
        QApplication::restoreOverrideCursor();
        KMessageBox::error(this, i18n("ark cannot create an archive of that type.\n\n  [Hint: The filename should have an extension such as '.zip' to\n  indicate the archive type. Please see the help pages for\nmore information on supported archive formats.]"));
    }
}

//////////////////////////////////////////////////////////////////////
//////////////////////// openArchive /////////////////////////////////
//////////////////////////////////////////////////////////////////////

void
ArkWidget::openArchive( const QString & _filename )
{
    Arch *newArch = 0;
    ArchType archtype;
    if ( m_openAsMimeType.isNull() )
    {
        archtype = ArchiveFormatInfo::self()->archTypeForURL( m_url );
        if ( ArchiveFormatInfo::self()->wasUnknownExtension() )
        {
            ArchiveFormatDlg * dlg = new ArchiveFormatDlg( this, KMimeType::findByURL( m_url )->name() );
            if ( !dlg->exec() == QDialog::Accepted )
            {
                emit setWindowCaption( QString::null );
                emit removeRecentURL( _filename );
                delete dlg;
                file_close();
                return;
            }
            m_openAsMimeType = dlg->mimeType();
            archtype = ArchiveFormatInfo::self()->archTypeForMimeType( m_openAsMimeType );
            delete dlg;
        }
    }
    else
    {
       archtype = ArchiveFormatInfo::self()->archTypeForMimeType( m_openAsMimeType );
    }

    kdDebug( 1601 ) << "m_openAsMimeType is: " << m_openAsMimeType << endl;
    if( 0 == ( newArch = Arch::archFactory( archtype, m_settings, this,
                                            _filename, m_openAsMimeType) ) )
    {
        emit setWindowCaption( QString::null );
        emit removeRecentURL( _filename );
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
    newArch->open();
}

void
ArkWidget::slotOpen( Arch *_newarch, bool _success, const QString & _filename, int )
{
    kdDebug(1601) << "+ArkWidget::slotOpen" << endl;
    archiveContent->setUpdatesEnabled(true);
    archiveContent->triggerUpdate();

    if ( _success )
    {
        QFileInfo fi( _filename );
        QString path = fi.dirPath( true );
        m_settings->setLastOpenDir( path );
        QString dirtmp;
        QString directory("tmp.");
        dirtmp = locateLocal( "tmp", directory );

        if ( _filename.left( dirtmp.length() ) == dirtmp || !fi.isWritable() )
        {
            _newarch->setReadOnly(true);
            QApplication::restoreOverrideCursor(); // no wait cursor during a msg box (David)
            KMessageBox::information(this, i18n("This archive is read-only. If you want to save it under a new name, go to the File menu and select Save As."), i18n("Information"), "ReadOnlyArchive");
            QApplication::setOverrideCursor( waitCursor );
        }
        //      emit setWindowCaption( _filename );
        arch = _newarch;
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
        emit removeRecentURL( _filename );
        emit setWindowCaption( QString::null );
        QApplication::restoreOverrideCursor();
        KMessageBox::error( this, i18n( "An error occured while trying to open the archive %1" ).arg( _filename ) );

        if ( m_extractOnly )
            emit request_file_quit();
    }

    fixEnables();
    QApplication::restoreOverrideCursor();

    kdDebug(1601) << "-ArkWidget::slotOpen" << endl;
}

#include "arkwidget.moc"
