/*

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org
 1999-2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)
 2001: Corel Corporation (author: Michael Jarrett, michaelj@corel.com)
 2001: Roberto Selbach Teixeira <maragato@conectiva.com>
 2003: Georg Robbers <Georg.Robbers@urz.uni-hd.de>

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
#include <QByteArray>

// KDE includes
#include <kapplication.h>
#include <kdebug.h>
#include <kde_file.h>
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

static char *makeAccessString(mode_t mode);

TarArch::TarArch( ArkWidget *_gui,
                  const QString & _filename, const QString & _openAsMimeType)
  : Arch( _gui, _filename), createTmpInProgress(false),
    updateInProgress(false), deleteInProgress(false), fd(NULL),
    m_pTmpProc( NULL ), m_pTmpProc2( NULL ), tarptr( NULL ), failed( false )
{
    m_tmpDir = NULL;
    m_dotslash = false;
    m_filesToAdd = m_filesToRemove = QStringList();
    kDebug(1601) << "+TarArch::TarArch" << endl;
    m_archiver_program = ArkSettings::tarExe();
    m_unarchiver_program = QString::null;
    verifyUtilityIsAvailable(m_archiver_program, m_unarchiver_program);

    m_fileMimeType = _openAsMimeType;
    if ( m_fileMimeType.isNull() )
        m_fileMimeType = KMimeType::findByPath( _filename )->name();

    kDebug(1601) << "TarArch::TarArch:  mimetype is " << m_fileMimeType << endl;

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

        kDebug(1601) << "Tmpfile will be " << tmpfile << "\n" << endl;
    }
    kDebug(1601) << "-TarArch::TarArch" << endl;
}

TarArch::~TarArch()
{
    if ( m_tmpDir )
        delete m_tmpDir;
    if ( tarptr )
        delete tarptr;
}

int TarArch::getEditFlag()
{
  return Arch::Extract;
}

void TarArch::updateArch()
{
  kDebug(1601) << "+TarArch::updateArch" << endl;
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
  kDebug(1601) << "-TarArch::updateArch" << endl;
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
      kWarning( 1601 ) << "trouble updating tar archive" << endl;
      //kFatal( 1601 ) << "trouble updating tar archive" << endl;
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
    kDebug(1601) << "+TarArch::open" << endl;
    if ( compressed )
        QFile::remove(tmpfile); // just to make sure
    setHeaders();

//  m_shellErrorData = "";
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
            || m_fileMimeType == "application/x-tbz" )
    {
        QString type = ( m_fileMimeType == "application/x-tgz" )
            ? "application/x-gzip" : "application/x-bzip2";
        tarptr = new KTar ( m_filename, type );
        openFirstCreateTempDone();
    }
    else if ( !compressed )
    {
        tarptr = new KTar( m_filename );
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
        tarptr = new KTar(tmpfile);
    }

    failed = !tarptr->open( QIODevice::ReadOnly );
    // failed is false for a tar.gz archive opened as tar.bz2 ?
    // but one should be able to open empty archives...
    // failed = failed || ( tarptr->directory()->entries().isEmpty();
    if( failed && ( getUnCompressor() == QString("gunzip")
                || getUnCompressor() == QString("bunzip2") ) )
    {
        delete tarptr;
        tarptr = NULL;
        connect( this, SIGNAL( createTempDone() ), this, SLOT( openSecondCreateTempDone() ) );
        createTmp();
    }
    else
        openSecondCreateTempDone();
}

void TarArch::openSecondCreateTempDone()
{
    if( failed && ( getUnCompressor() == QString("gunzip")
                || getUnCompressor() == QString("bunzip2") ) )
    {
        disconnect( this, SIGNAL( createTempDone() ), this, SLOT( openSecondCreateTempDone() ) );
        kDebug(1601)  << "Creating KTar from failed IO_RW " << m_filename <<
            " using uncompressor " << getUnCompressor() << endl;
        if ( KMimeType::findByFileContent( tmpfile )->name() != "application/x-zerosize" )
        {
            tarptr = new KTar(tmpfile);
            failed = !tarptr->open(QIODevice::ReadOnly);
        }
    }

    // sigOpen might lead to application exit, delete tarptr before sigOpen
    // to avoid double deletion
    if( failed )
    {
        kDebug(1601)  << "Failed to uncompress and open." << endl;
        delete tarptr;
        tarptr = NULL;
        emit sigOpen(this, false, QString::null, 0 );
    }
    else
    {
        processDir(tarptr->directory(), "");
        delete tarptr;
        tarptr = NULL;
        // because we aren't using the KProcess method, we have to emit this
        // ourselves.
        emit sigOpen(this, true, m_filename,
                Arch::Extract | Arch::Delete | Arch::Add | Arch::View );
    }
}

void TarArch::slotListingDone(KProcess *_kp)
{
  const QString list = getLastShellOutput();
  FileListView *flv = m_gui->fileList();
  if (flv!=NULL && flv->totalFiles()>0)
  {
    const QString firstfile = ((FileLVI *) flv->firstChild())->fileName();
    if (list.contains(QRegExp(QString("\\s\\./%1[/\\n]").arg(firstfile))))
    {
      m_dotslash = true;
      kDebug(1601) << k_funcinfo << "archive has dot-slash" << endl;
    }
    else
    {
      if (list.contains(QRegExp(QString("\\s%1[/\\n]").arg(firstfile))))
      {
        // archive doesn't have dot-slash
        m_dotslash = false;
      }
      else
      {
        kDebug(1601) << k_funcinfo << "cannot match '" << firstfile << "' in listing!" << endl;
      }
    }
  }

  delete _kp;
  _kp = m_currentProcess = NULL;
}

void TarArch::processDir(const KArchiveDirectory *tardir, const QString & root)
  // process a KTarDirectory. Called recursively for directories within
  // directories, etc. Prepends to filename root, for relative pathnames.
{
  QStringList list = tardir->entries();

  for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it )
    {
      const KArchiveEntry* tarEntry = tardir->entry((*it));
      if (tarEntry == NULL)
        return;

      QStringList col_list;
      QString name;
      if (root.isEmpty() || root.isNull())
        name = tarEntry->name();
      else
        name = root + tarEntry->name();
      if ( !tarEntry->isFile() )
        name += '/';
      col_list.append( name );
      QString perms = makeAccessString(tarEntry->permissions());
      if (!tarEntry->isFile())
        perms = "d" + perms;
      else if (!tarEntry->symlink().isEmpty())
        perms = "l" + perms;
      else
        perms = "-" + perms;
      col_list.append(perms);
      col_list.append( tarEntry->user() );
      col_list.append( tarEntry->group() );
      QString strSize = "0";
      if (tarEntry->isFile())
        {
          strSize.sprintf("%ld", ( long int )((KArchiveFile *)tarEntry)->size());
        }
      col_list.append(strSize);
      QString timestamp = tarEntry->datetime().toString(Qt::ISODate);
      col_list.append(timestamp);
      col_list.append(tarEntry->symlink());
      m_gui->fileList()->addItem(col_list); // send the entry to the GUI

      // if it isn't a file, it's a directory - process it.
      // remember that name is root + / + the name of the directory
      if (!tarEntry->isFile())
        processDir( (KArchiveDirectory *)tarEntry, name );
      kapp->processEvents( QEventLoop::ExcludeUserInputEvents, 20 );
    }
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
                temp.open( QIODevice::ReadWrite );
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
            kDebug(1601) << "Uncompressor is " << strUncompressor << endl;
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
            kDebug(1601) << "Temp tar already there..." << endl;
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
      //kFatal( 1601 ) << "Trouble writing to archive(createTmpProgress)" << endl;
      kWarning( 1601 ) << "Trouble writing to archive(createTmpProgress)" << endl;
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
    KUrl url( *iter );
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

      kDebug(1601) << "Old file: " << oldFileMTime.date().year() << "-" <<
        oldFileMTime.date().month() << "-" << oldFileMTime.date().day() <<
        " " << oldFileMTime.time().hour() << ":" <<
        oldFileMTime.time().minute() << ":" << oldFileMTime.time().second() <<
        endl;
      kDebug(1601) << "New file: " << addFileMTime.date().year()  << "-" <<
        addFileMTime.date().month()  << "-" << addFileMTime.date().day() <<
        " " << addFileMTime.time().hour()  << ":" <<
        addFileMTime.time().minute() << ":" << addFileMTime.time().second() <<
        endl;

      if (oldFileMTime >= addFileMTime)
      {
        kDebug(1601) << "Old time is newer or same" << endl;
        continue; // don't add this file to the list to be deleted.
      }
    }
    list.append(str);

    kDebug(1601) << "To delete: " << str << endl;
  }
  if(!list.isEmpty())
    remove(&list);
  else
    emit removeDone();
}


void TarArch::addFile( const QStringList&  urls )
{
  kDebug(1601) << "+TarArch::addFile" << ( urls.first() ) << endl;
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
  KUrl url( urls->first() );
  QDir::setCurrent( url.directory() );
  for (iter = urls->begin(); iter != urls->end(); ++iter )
  {
    KUrl fileURL( *iter );
    *kp << fileURL.fileName();
  }

  // debugging info
  QList<QByteArray> list = kp->args();
  QList<QByteArray>::Iterator strTemp;
  for ( strTemp=list.begin(); strTemp != list.end(); ++strTemp )
    {
      kDebug(1601) << *strTemp << " " << endl;
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

  kDebug(1601) << "-TarArch::addFile" << endl;
}

void TarArch::slotAddFinished(KProcess *_kp)
{
  kDebug(1601) << "+TarArch::slotAddFinished" << endl;

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
  kDebug(1601) << "+TarArch::addFinishedUpdateDone" << endl;
  if ( compressed )
    disconnect( this, SIGNAL( updateDone() ), this, SLOT( addFinishedUpdateDone() ) );
  Arch::slotAddExited( m_pTmpProc ); // this will delete _kp
  m_pTmpProc = NULL;
  kDebug(1601) << "-TarArch::addFinishedUpdateDone" << endl;
}

void TarArch::unarchFileInternal()
{
  kDebug(1601) << "+TarArch::unarchFile" << endl;
  QString dest;

  if (m_destDir.isEmpty() || m_destDir.isNull())
    {
      kError(1601) << "There was no extract directory given." << endl;
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

  kDebug(1601) << "Options were: " << options << endl;
  *kp << options << m_filename << "-C" << dest;

  // if the list is empty, no filenames go on the command line,
  // and we then extract everything in the archive.
  if (m_fileList)
    {
      for ( QStringList::Iterator it = m_fileList->begin();
            it != m_fileList->end(); ++it )
        {
	    *kp << QString(m_dotslash ? "./" : "")+(*it);
//	    *kp << (*it);/*.latin1() ;*/
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

  kDebug(1601) << "+TarArch::unarchFile" << endl;
}

void TarArch::remove(QStringList *list)
{
  kDebug(1601) << "+Tar::remove" << endl;
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
//      kDebug(1601) << *it << endl;
//      *kp << *it;
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

  kDebug(1601) << "-Tar::remove" << endl;
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
  kDebug(1601) << "Open finshed" << endl;
}

void TarArch::createTmpFinished( KProcess *_kp )
{
  kDebug(1601) << "+TarArch::createTmpFinished" << endl;

  createTmpInProgress = false;
  fclose(fd);
  delete _kp;
  _kp = m_currentProcess = NULL;


  kDebug(1601) << "-TarArch::createTmpFinished" << endl;
  emit createTempDone();
}

void TarArch::updateFinished( KProcess *_kp )
{
  kDebug(1601) << "+TarArch::updateFinished" << endl;
  fclose(fd);
  updateInProgress = false;
  delete _kp;
  _kp = m_currentProcess = NULL;

  kDebug(1601) << "-TarArch::updateFinished" << endl;
  emit updateDone();
}

////////////////////////////////////////////////////////////////////////
/////////////////// some helper functions
///////////////////////////////////////////////////////////////////////

// copied from KonqTreeViewItem::makeAccessString()
static char *makeAccessString(mode_t mode)
{
  static char buffer[10];

  char uxbit,gxbit,oxbit;

  if ( (mode & (S_IXUSR|S_ISUID)) == (S_IXUSR|S_ISUID) )
    uxbit = 's';
  else if ( (mode & (S_IXUSR|S_ISUID)) == S_ISUID )
    uxbit = 'S';
  else if ( (mode & (S_IXUSR|S_ISUID)) == S_IXUSR )
    uxbit = 'x';
  else
    uxbit = '-';

  if ( (mode & (S_IXGRP|S_ISGID)) == (S_IXGRP|S_ISGID) )
    gxbit = 's';
  else if ( (mode & (S_IXGRP|S_ISGID)) == S_ISGID )
    gxbit = 'S';
  else if ( (mode & (S_IXGRP|S_ISGID)) == S_IXGRP )
    gxbit = 'x';
  else
    gxbit = '-';

  if ( (mode & (S_IXOTH|S_ISVTX)) == (S_IXOTH|S_ISVTX) )
    oxbit = 't';
  else if ( (mode & (S_IXOTH|S_ISVTX)) == S_ISVTX )
    oxbit = 'T';
  else if ( (mode & (S_IXOTH|S_ISVTX)) == S_IXOTH )
    oxbit = 'x';
  else
    oxbit = '-';

  buffer[0] = ((( mode & S_IRUSR ) == S_IRUSR ) ? 'r' : '-' );
  buffer[1] = ((( mode & S_IWUSR ) == S_IWUSR ) ? 'w' : '-' );
  buffer[2] = uxbit;
  buffer[3] = ((( mode & S_IRGRP ) == S_IRGRP ) ? 'r' : '-' );
  buffer[4] = ((( mode & S_IWGRP ) == S_IWGRP ) ? 'w' : '-' );
  buffer[5] = gxbit;
  buffer[6] = ((( mode & S_IROTH ) == S_IROTH ) ? 'r' : '-' );
  buffer[7] = ((( mode & S_IWOTH ) == S_IWOTH ) ? 'w' : '-' );
  buffer[8] = oxbit;
  buffer[9] = 0;

  return buffer;
}

#include "tar.moc"
