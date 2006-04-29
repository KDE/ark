/*

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org
 1999-2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)
 2001: Corel Corporation (author: Michael Jarrett, michaelj@corel.com)
 2001: Roberto Selbach Teixeira <maragato@conectiva.com>
 2003: Georg Robbers <Georg.Robbers@urz.uni-hd.de>
 2006: Henrique Pinto <henrique.pinto@kdemail.net>

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

// Note: When maintaining tar files with ark, the user should be
// aware that these options have been improved (IMHO). When you append a file
// to a tarchive, tar does not check if the file exists already, and just
// tacks the new one on the end. ark deletes the old one.
// When you update a file that exists in a tarchive, it does check if
// it exists, but once again, it creates a duplicate at the end (only if
// the file is newer though). ark deletes the old one in this case as well.
//
// Basically, tar files are great for creating and extracting, but
// not especially for maintaining. The original purpose of a tar was of
// course, for tape backups, so this is not so surprising!      -Emily
//

// C includes
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

// Qt includes
#include <qdir.h>
#include <qregexp.h>
#include <qeventloop.h>

// KDE includes
#include <kapplication.h>
#include <kdebug.h>
#include <klargefile.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <ktempfile.h>
#include <kmimetype.h>
#include <kstandarddirs.h>
#include <ktempdir.h>
#include <kprocess.h>
#include <ktar.h>

// ark includes
#include "arkwidget.h"
#include "settings.h"
#include "tar.h"
#include "filelistview.h"
#include "tarlistingthread.h"

TarArch::TarArch( ArkWidget *_gui,
                  const QString & _filename, const QString & _openAsMimeType)
  : Arch( _gui, _filename), m_tmpDir( 0 ), createTmpInProgress(false),
    updateInProgress(false), deleteInProgress(false), fd(0),
    m_pTmpProc( 0 ), m_pTmpProc2( 0 ), failed( false ), 
    m_dotslash( false ), m_listingThread( 0 )
{
    m_filesToAdd = m_filesToRemove = QStringList();
    m_archiver_program = m_unarchiver_program = ArkSettings::tarExe();
    verifyCompressUtilityIsAvailable( m_archiver_program );
    verifyUncompressUtilityIsAvailable( m_unarchiver_program );

    m_fileMimeType = _openAsMimeType;
    if ( m_fileMimeType.isNull() )
        m_fileMimeType = KMimeType::findByPath( _filename )->name();

    kdDebug(1601) << "TarArch::TarArch:  mimetype is " << m_fileMimeType << endl;

    if ( m_fileMimeType == "application/x-tbz2" )
    {
        // ark treats .tar.bz2 as x-tbz, instead of duplicating the mimetype
        // let's just alias it to the one we already handle.
        m_fileMimeType = "application/x-tbz";
    }

    if ( m_fileMimeType == "application/x-tar" )
    {
        compressed = false;
    }
    else
    {
        compressed = true;
        m_tmpDir = new KTempDir( _gui->tmpDir()
                                 + QString::fromLatin1( "temp_tar" ) );
        m_tmpDir->setAutoDelete( true );
        m_tmpDir->qDir()->cd( m_tmpDir->name() );
        // build the temp file name
        KTempFile *pTempFile = new KTempFile( m_tmpDir->name(),
                QString::fromLatin1(".tar") );

        tmpfile = pTempFile->name();
        delete pTempFile;

        kdDebug(1601) << "Tmpfile will be " << tmpfile << "\n" << endl;
    }
}

TarArch::~TarArch()
{
    delete m_tmpDir;
    m_tmpDir = 0;

    if ( m_listingThread && m_listingThread->finished() != true )
    {
      m_listingThread->wait();
      delete m_listingThread;
      m_listingThread = 0;
    }
}

int TarArch::getEditFlag()
{
    return Arch::Extract;
}

void TarArch::updateArch()
{
  if (compressed)
    {
      updateInProgress = true;
      int f_desc = KDE_open(QFile::encodeName(m_filename), O_CREAT | O_TRUNC | O_WRONLY, 0666);
      if (f_desc != -1)
          fd = fdopen( f_desc, "w" );
      else
          fd = NULL;

      KProcess *kp = m_currentProcess = new KProcess;
      kp->clearArguments();
      KProcess::Communication flag = KProcess::AllOutput;
      if ( getCompressor() == "lzop" )
      {
        kp->setUsePty( KProcess::Stdin, false );
        flag = KProcess::Stdout;
      }
      if ( !getCompressor().isNull() )
          *kp << getCompressor() << "-c" << tmpfile;
      else
          *kp << "cat" << tmpfile;


      connect(kp, SIGNAL(receivedStdout(KProcess*, char*, int)),
              this, SLOT(updateProgress( KProcess *, char *, int )));
      connect( kp, SIGNAL(receivedStderr(KProcess*, char*, int)),
               (Arch *)this, SLOT(slotReceivedOutput(KProcess*, char*, int)));

      connect(kp, SIGNAL(processExited(KProcess *)),
               this, SLOT(updateFinished(KProcess *)) );

      if ( !fd || kp->start(KProcess::NotifyOnExit, flag) == false)
        {
          KMessageBox::error(0, i18n("Trouble writing to the archive..."));
          emit updateDone();
        }
    }
}

void TarArch::updateProgress( KProcess * _proc, char *_buffer, int _bufflen )
{
  // we're trying to capture the output of a command like this
  //    gzip -c myarch.tar
  // and feed the output to the archive
  int size;
  size = fwrite(_buffer, 1, _bufflen, fd);
  if (size != _bufflen)
    {
      _proc->kill();
      KMessageBox::error(0, i18n("Trouble writing to the archive..."));
      kdWarning( 1601 ) << "trouble updating tar archive" << endl;
      //kdFatal( 1601 ) << "trouble updating tar archive" << endl;
    }
}



QString TarArch::getCompressor()
{
    if ( m_fileMimeType == "application/x-tarz" )
        return QString( "compress" );

    if ( m_fileMimeType == "application/x-tgz" )
        return QString( "gzip" );

    if (  m_fileMimeType == "application/x-tbz" )
        return QString( "bzip2" );

    if( m_fileMimeType == "application/x-tzo" )
        return QString( "lzop" );

    return QString::null;
}


QString TarArch::getUnCompressor()
{
    if ( m_fileMimeType == "application/x-tarz" )
        return QString( "uncompress" );

    if ( m_fileMimeType == "application/x-tgz" )
        return QString( "gunzip" );

    if (  m_fileMimeType == "application/x-tbz" )
        return QString( "bunzip2" );

    if( m_fileMimeType == "application/x-tzo" )
        return QString( "lzop" );

    return QString::null;
}

void
TarArch::open()
{
    if ( compressed )
        QFile::remove(tmpfile); // just to make sure
    setHeaders();

    clearShellOutput();

    // might as well plunk the output of tar -tvf in the shell output window...
    //
    // Now it's essential - used later to decide whether pathnames in the
    // tar archive are plain or start with "./"
    KProcess *kp = m_currentProcess = new KProcess;

    *kp << m_archiver_program;

    if ( compressed )
    {
        *kp << "--use-compress-program=" + getUnCompressor();
    }

    *kp << "-tvf" << m_filename;

    m_buffer = "";
    m_header_removed = false;
    m_finished = false;

    connect(kp, SIGNAL(processExited(KProcess *)),
            this, SLOT(slotListingDone(KProcess *)));
    connect(kp, SIGNAL(receivedStdout(KProcess*, char*, int)),
        this, SLOT(slotReceivedOutput( KProcess *, char *, int )));
    connect( kp, SIGNAL(receivedStderr(KProcess*, char*, int)),
        this, SLOT(slotReceivedOutput(KProcess*, char*, int)));

    if (kp->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false)
    {
        KMessageBox::error( 0, i18n("Could not start a subprocess.") );
    }

    // We list afterwards because we want the signals at the end
    // This unconfuses Extract Here somewhat

    if ( m_fileMimeType == "application/x-tgz"
            || m_fileMimeType == "application/x-tbz" || !compressed )
    {
        openFirstCreateTempDone();
    }
    else
    {
        connect( this, SIGNAL( createTempDone() ), this, SLOT( openFirstCreateTempDone() ) );
        createTmp();
    }
}

void TarArch::openFirstCreateTempDone()
{
    if ( compressed && ( m_fileMimeType != "application/x-tgz" )
            && ( m_fileMimeType != "application/x-tbz" ) )
    {
        disconnect( this, SIGNAL( createTempDone() ), this, SLOT( openFirstCreateTempDone() ) );
    }

    Q_ASSERT( !m_listingThread );
    m_listingThread = new TarListingThread( this, m_filename );
    m_listingThread->start();
}

void TarArch::slotListingDone(KProcess *_kp)
{
  const QString list = getLastShellOutput();
  FileListView *flv = m_gui->fileList();
  if (flv!=NULL && flv->totalFiles()>0)
  {
    const QString firstfile = ((FileLVI *) flv->firstChild())->fileName();
    if (list.find(QRegExp(QString("\\s\\./%1[/\\n]").arg(firstfile)))>=0)
    {
      m_dotslash = true;
      kdDebug(1601) << k_funcinfo << "archive has dot-slash" << endl;
    }
    else
    {
      if (list.find(QRegExp(QString("\\s%1[/\\n]").arg(firstfile)))>=0)
      {
        // archive doesn't have dot-slash
        m_dotslash = false;
      }
      else
      {
        kdDebug(1601) << k_funcinfo << "cannot match '" << firstfile << "' in listing!" << endl;
      }
    }
  }

  delete _kp;
  _kp = m_currentProcess = NULL;
}

void TarArch::create()
{
  emit sigCreate(this, true, m_filename,
                 Arch::Extract | Arch::Delete | Arch::Add
                  | Arch::View);
}

void TarArch::setHeaders()
{
  ColumnList list;

  list.append(FILENAME_COLUMN);
  list.append(PERMISSION_COLUMN);
  list.append(OWNER_COLUMN);
  list.append(GROUP_COLUMN);
  list.append(SIZE_COLUMN);
  list.append(TIMESTAMP_COLUMN);
  list.append(LINK_COLUMN);

  emit headers( list );
}

void TarArch::createTmp()
{
    if ( compressed )
    {
        if ( !QFile::exists(tmpfile) )
        {
            QString strUncompressor = getUnCompressor();
            // at least lzop doesn't want to pipe zerosize/nonexistent files
            QFile originalFile( m_filename );
            if ( strUncompressor != "gunzip" && strUncompressor !="bunzip2" &&
                ( !originalFile.exists() || originalFile.size() == 0 ) )
            {
                QFile temp( tmpfile );
                temp.open( IO_ReadWrite );
                temp.close();
                emit createTempDone();
                return;
            }
            // the tmpfile does not yet exist, so we create it.
            createTmpInProgress = true;
            int f_desc = KDE_open(QFile::encodeName(tmpfile), O_CREAT | O_TRUNC | O_WRONLY, 0666);
            if (f_desc != -1)
                fd = fdopen( f_desc, "w" );
            else
                fd = NULL;

            KProcess *kp = m_currentProcess = new KProcess;
            kp->clearArguments();
            kdDebug(1601) << "Uncompressor is " << strUncompressor << endl;
            *kp << strUncompressor;
            KProcess::Communication flag = KProcess::AllOutput;
            if (strUncompressor == "lzop")
            {
                // setting up a pty for lzop, since it doesn't like stdin to
                // be /dev/null ( "no filename allowed when reading from stdin" )
                // - but it used to work without this ? ( Feb 13, 2003 )
                kp->setUsePty( KProcess::Stdin, false );
                flag = KProcess::Stdout;
                *kp << "-d";
            }
            *kp << "-c" << m_filename;

            connect(kp, SIGNAL(processExited(KProcess *)),
                    this, SLOT(createTmpFinished(KProcess *)));
            connect(kp, SIGNAL(receivedStdout(KProcess*, char*, int)),
                    this, SLOT(createTmpProgress( KProcess *, char *, int )));
            connect( kp, SIGNAL(receivedStderr(KProcess*, char*, int)),
                    this, SLOT(slotReceivedOutput(KProcess*, char*, int)));
            if (kp->start(KProcess::NotifyOnExit, flag ) == false)
            {
                KMessageBox::error(0, i18n("Unable to fork a decompressor"));
		emit sigOpen( this, false, QString::null, 0 );
            }
        }
        else
        {
            emit createTempDone();
            kdDebug(1601) << "Temp tar already there..." << endl;
        }
    }
    else
    {
        emit createTempDone();
    }
}

void TarArch::createTmpProgress( KProcess * _proc, char *_buffer, int _bufflen )
{
  // we're trying to capture the output of a command like this
  //    gunzip -c myarch.tar.gz
  // and put the output into tmpfile.

  int size;
  size = fwrite(_buffer, 1, _bufflen, fd);
  if (size != _bufflen)
    {
      _proc->kill();
      KMessageBox::error(0, i18n("Trouble writing to the tempfile..."));
      //kdFatal( 1601 ) << "Trouble writing to archive(createTmpProgress)" << endl;
      kdWarning( 1601 ) << "Trouble writing to archive(createTmpProgress)" << endl;
      //exit(99);
    }
}

void TarArch::deleteOldFiles(const QStringList &urls, bool bAddOnlyNew)
  // because tar is broken. Used when appending: see addFile.
{
  QStringList list;
  QString str;

  QStringList::ConstIterator iter;
  for (iter = urls.begin(); iter != urls.end(); ++iter )
  {
    KURL url( *iter );
    // find the file entry in the archive listing
    const FileLVI * lv = m_gui->fileList()->item( url.fileName() );
    if ( !lv ) // it isn't in there, so skip it.
      continue;

    if (bAddOnlyNew)
    {
      // compare timestamps. If the file to be added is newer, delete the
      // old. Otherwise we aren't adding it anyway, so we can go on to the next
      // file with a "continue".

      QFileInfo fileInfo( url.path() );
      QDateTime addFileMTime = fileInfo.lastModified();
      QDateTime oldFileMTime = lv->timeStamp();

      kdDebug(1601) << "Old file: " << oldFileMTime.date().year() << '-' <<
        oldFileMTime.date().month() << '-' << oldFileMTime.date().day() <<
        ' ' << oldFileMTime.time().hour() << ':' <<
        oldFileMTime.time().minute() << ':' << oldFileMTime.time().second() <<
        endl;
      kdDebug(1601) << "New file: " << addFileMTime.date().year()  << '-' <<
        addFileMTime.date().month()  << '-' << addFileMTime.date().day() <<
        ' ' << addFileMTime.time().hour()  << ':' <<
        addFileMTime.time().minute() << ':' << addFileMTime.time().second() <<
        endl;

      if (oldFileMTime >= addFileMTime)
      {
        kdDebug(1601) << "Old time is newer or same" << endl;
        continue; // don't add this file to the list to be deleted.
      }
    }
    list.append(str);

    kdDebug(1601) << "To delete: " << str << endl;
  }
  if(!list.isEmpty())
    remove(&list);
  else
    emit removeDone();
}


void TarArch::addFile( const QStringList&  urls )
{
  m_filesToAdd = urls;
  // tar is broken. If you add a file that's already there, it gives you
  // two entries for that name, whether you --append or --update. If you
  // extract by name, it will give you
  // the first one. If you extract all, the second one will overwrite the
  // first. So we'll first delete all the old files matching the names of
  // those in urls.
  m_bNotifyWhenDeleteFails = false;
  connect( this, SIGNAL( removeDone() ), this, SLOT( deleteOldFilesDone() ) );
  deleteOldFiles(urls, ArkSettings::replaceOnlyWithNewer());
}

void TarArch::deleteOldFilesDone()
{
  disconnect( this, SIGNAL( removeDone() ), this, SLOT( deleteOldFilesDone() ) );
  m_bNotifyWhenDeleteFails = true;

  connect( this, SIGNAL( createTempDone() ), this, SLOT( addFileCreateTempDone() ) );
  createTmp();
}

void TarArch::addFileCreateTempDone()
{
  disconnect( this, SIGNAL( createTempDone() ), this, SLOT( addFileCreateTempDone() ) );
  QStringList * urls = &m_filesToAdd;

  KProcess *kp = m_currentProcess = new KProcess;
  *kp << m_archiver_program;

  if( ArkSettings::replaceOnlyWithNewer())
    *kp << "uvf";
  else
    *kp << "rvf";

  if (compressed)
    *kp << tmpfile;
  else
    *kp << m_filename;

  QStringList::ConstIterator iter;
  KURL url( urls->first() );
  QDir::setCurrent( url.directory() );
  for (iter = urls->begin(); iter != urls->end(); ++iter )
  {
    KURL fileURL( *iter );
    *kp << fileURL.fileName();
  }

  // debugging info
  QValueList<QCString> list = kp->args();
  QValueList<QCString>::Iterator strTemp;
  for ( strTemp=list.begin(); strTemp != list.end(); ++strTemp )
    {
      kdDebug(1601) << *strTemp << " " << endl;
    }

  connect( kp, SIGNAL(receivedStdout(KProcess*, char*, int)),
           this, SLOT(slotReceivedOutput(KProcess*, char*, int)));
  connect( kp, SIGNAL(receivedStderr(KProcess*, char*, int)),
           this, SLOT(slotReceivedOutput(KProcess*, char*, int)));

  connect( kp, SIGNAL(processExited(KProcess*)), this,
           SLOT(slotAddFinished(KProcess*)));

  if (kp->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false)
    {
      KMessageBox::error( 0, i18n("Could not start a subprocess.") );
      emit sigAdd(false);
    }
}

void TarArch::slotAddFinished(KProcess *_kp)
{
  disconnect( _kp, SIGNAL(processExited(KProcess*)), this,
              SLOT(slotAddFinished(KProcess*)));
  m_pTmpProc = _kp;
  m_filesToAdd = QStringList();
  if ( compressed )
  {
    connect( this, SIGNAL( updateDone() ), this, SLOT( addFinishedUpdateDone() ) );
    updateArch();
  }
  else
    addFinishedUpdateDone();
}

void TarArch::addFinishedUpdateDone()
{
  if ( compressed )
    disconnect( this, SIGNAL( updateDone() ), this, SLOT( addFinishedUpdateDone() ) );
  Arch::slotAddExited( m_pTmpProc ); // this will delete _kp
  m_pTmpProc = NULL;
}

void TarArch::unarchFileInternal()
{
  QString dest;

  if (m_destDir.isEmpty() || m_destDir.isNull())
    {
      kdError(1601) << "There was no extract directory given." << endl;
      return;
    }
  else dest = m_destDir;

  QString tmp;

  KProcess *kp = m_currentProcess = new KProcess;
  kp->clearArguments();

  *kp << m_archiver_program;
  if (compressed)
    *kp << "--use-compress-program="+getUnCompressor();

  QString options = "-x";
  if (!ArkSettings::extractOverwrite())
    options += "k";
  if (ArkSettings::preservePerms())
    options += "p";
  options += "f";

  kdDebug(1601) << "Options were: " << options << endl;
  *kp << options << m_filename << "-C" << dest;

  // if the list is empty, no filenames go on the command line,
  // and we then extract everything in the archive.
  if (m_fileList)
    {
      for ( QStringList::Iterator it = m_fileList->begin();
            it != m_fileList->end(); ++it )
        {
            *kp << QString(m_dotslash ? "./" : "")+(*it);
        }
    }

  connect( kp, SIGNAL(receivedStdout(KProcess*, char*, int)),
           this, SLOT(slotReceivedOutput(KProcess*, char*, int)));
  connect( kp, SIGNAL(receivedStderr(KProcess*, char*, int)),
           this, SLOT(slotReceivedOutput(KProcess*, char*, int)));

  connect( kp, SIGNAL(processExited(KProcess*)), this,
           SLOT(slotExtractExited(KProcess*)));

  if (kp->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false)
    {
      KMessageBox::error( 0, i18n("Could not start a subprocess.") );
      emit sigExtract(false);
    }

}

void TarArch::remove(QStringList *list)
{
  deleteInProgress = true;
  m_filesToRemove = *list;
  connect( this, SIGNAL( createTempDone() ), this, SLOT( removeCreateTempDone() ) );
  createTmp();
}

void TarArch::removeCreateTempDone()
{
  disconnect( this, SIGNAL( createTempDone() ), this, SLOT( removeCreateTempDone() ) );

  QString name, tmp;
  KProcess *kp = m_currentProcess = new KProcess;
  kp->clearArguments();
  *kp << m_archiver_program << "--delete" << "-f" ;
  if (compressed)
    *kp << tmpfile;
  else
    *kp << m_filename;

  QStringList::Iterator it = m_filesToRemove.begin();
  for ( ; it != m_filesToRemove.end(); ++it )
    {
        *kp << QString(m_dotslash ? "./" : "")+(*it);
    }
  m_filesToRemove = QStringList();

  connect( kp, SIGNAL(receivedStdout(KProcess*, char*, int)),
           this, SLOT(slotReceivedOutput(KProcess*, char*, int)));
  connect( kp, SIGNAL(receivedStderr(KProcess*, char*, int)),
           this, SLOT(slotReceivedOutput(KProcess*, char*, int)));

  connect( kp, SIGNAL(processExited(KProcess*)), this,
           SLOT(slotDeleteExited(KProcess*)));

  if (kp->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false)
    {
      KMessageBox::error( 0, i18n("Could not start a subprocess.") );
      emit sigDelete(false);
    }
}

void TarArch::slotDeleteExited(KProcess *_kp)
{
  m_pTmpProc2 = _kp;
  if ( compressed )
  {
    connect( this, SIGNAL( updateDone() ), this, SLOT( removeUpdateDone() ) );
    updateArch();
  }
  else
    removeUpdateDone();
}

void TarArch::removeUpdateDone()
{
  if ( compressed )
    disconnect( this, SIGNAL( updateDone() ), this, SLOT( removeUpdateDone() ) );

  deleteInProgress = false;
  emit removeDone();
  Arch::slotDeleteExited( m_pTmpProc2 );
  m_pTmpProc = NULL;
}

void TarArch::addDir(const QString & _dirName)
{
  QStringList list;
  list.append(_dirName);
  addFile(list);
}

void TarArch::openFinished( KProcess * )
{
  // do nothing
  // turn off busy light (when someone makes one)
  kdDebug(1601) << "Open finshed" << endl;
}

void TarArch::createTmpFinished( KProcess *_kp )
{
  createTmpInProgress = false;
  fclose(fd);
  delete _kp;
  _kp = m_currentProcess = NULL;

  emit createTempDone();
}

void TarArch::updateFinished( KProcess *_kp )
{
  fclose(fd);
  updateInProgress = false;
  delete _kp;
  _kp = m_currentProcess = NULL;

  emit updateDone();
}

void TarArch::customEvent( QCustomEvent *ev )
{
  if ( ev->type() == 65442 )
  {
    ListingEvent *event = static_cast<ListingEvent*>( ev );
    switch ( event->status() )
    {
      case ListingEvent::Normal:
        m_gui->fileList()->addItem( event->columns() );
        break;

      case ListingEvent::Error:
        m_listingThread->wait();
        delete m_listingThread;
        m_listingThread = 0;
        emit sigOpen( this, false, QString::null, 0 );
        break;

      case ListingEvent::ListingFinished:
        m_listingThread->wait();
        delete m_listingThread;
        m_listingThread = 0;
        emit sigOpen( this, true, m_filename,
                      Arch::Extract | Arch::Delete | Arch::Add | Arch::View );
    }
  }
}

#include "tar.moc"
// kate: space-indent on;
