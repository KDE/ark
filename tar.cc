/*

 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org
 1999-2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)
 2001: Corel Corporation (author: Michael Jarrett, michaelj@corel.com)
 2001: Roberto Selbach Teixeira <maragato@conectiva.com>

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

#include <kurl.h>
// Unsorted in qdir.h is used, but in some of the headers
// below it's defined, too. So I brought kurl.h to the top.
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Qt includes
#include <qstring.h>
#include <qstrlist.h>
#include <qregexp.h>
#include <qdatetime.h>
#include <qapplication.h>

// KDE includes
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <ktempfile.h>
#include <kmimemagic.h>
#include <kstddirs.h>
#include <kprocess.h>
#include <ktar.h>

// ark includes
#include "arch.h"
#include "arkwidgetbase.h"
#include "arksettings.h"
#include "arch.h"
#include "tar.h"


static char *makeAccessString(mode_t mode);
static QString makeTimeStamp(const QDateTime & dt);

TarArch::TarArch( ArkSettings *_settings, ArkWidgetBase *_gui,
                  const QString & _filename)
  : Arch(_settings, _gui, _filename), createTmpInProgress(false),
    updateInProgress(false), deleteInProgress(false), fd(NULL)
{
  kdDebug(1601) << "+TarArch::TarArch" << endl;
  m_archiver_program = m_settings->getTarCommand();
  m_unarchiver_program = QString::null;
  verifyUtilityIsAvailable(m_archiver_program, m_unarchiver_program);

  KMimeMagic *mimePtr = KMimeMagic::self();
  KMimeMagicResult * mimeResultPtr = mimePtr->findFileType(_filename);
  QString mimetype = mimeResultPtr->mimeType();
  kdDebug(1601) << "TarArch::TarArch:  mimetype is " << mimetype << endl;

  if (mimetype == "application/x-tar")
    {
      compressed = false;
    }
  else
    {
      compressed = true;
      QString tmpdir;
      QString directory;
      directory.sprintf("ark.%d/", getpid());
      tmpdir = locateLocal( "tmp", directory );
      //tmpdir.sprintf("/tmp/ark.%d", getpid());

      QString base = m_filename.right(m_filename.length()- 1 -
                                     m_filename.findRev("/"));
      base = base.left(base.findRev("."));

      // build the temp file name

      KTempFile *pTempFile = new KTempFile(tmpdir +
                                           QString::fromLocal8Bit("/temp_tar"),
                                           QString::fromLocal8Bit(".tar"));

      tmpfile = pTempFile->name();
      kdDebug(1601) << "Tmpfile will be " << tmpfile << "\n" << endl;
    }
  kdDebug(1601) << "-TarArch::TarArch" << endl;
}

TarArch::~TarArch()
{
  unlink( QFile::encodeName(tmpfile) );
}

int TarArch::getEditFlag()
{
  return Arch::Extract;
}

void TarArch::updateArch()
{
  kdDebug(1601) << "+TarArch::updateArch" << endl;
  if (compressed)
    {
      updateInProgress = true;
      fd = fopen( QFile::encodeName(m_filename), "w" );

      KProcess *kp = new KProcess;
      kp->clearArguments();
      if ( getCompressor() != QString::null )
          *kp << getCompressor() << "-c" << tmpfile;
      else
          *kp << "cat" << tmpfile;


      connect(kp, SIGNAL(receivedStdout(KProcess*, char*, int)),
              this, SLOT(updateProgress( KProcess *, char *, int )));
      connect( kp, SIGNAL(receivedStderr(KProcess*, char*, int)),
               (Arch *)this, SLOT(slotReceivedOutput(KProcess*, char*, int)));

      connect(kp, SIGNAL(processExited(KProcess *)),
               this, SLOT(updateFinished(KProcess *)) );

      if (kp->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false)
        {
          KMessageBox::error(0, i18n("Trouble writing to the archive..."));
        }
    }
  kdDebug(1601) << "-TarArch::updateArch" << endl;
}

void TarArch::updateProgress( KProcess *, char *_buffer, int _bufflen )
{
  // we're trying to capture the output of a command like this
  //    gzip -c myarch.tar
  // and feed the output to the archive
  int size;
  size = fwrite(_buffer, 1, _bufflen, fd);
  if (size != _bufflen)
    {
      KMessageBox::error(0, i18n("Trouble writing to the archive..."));
      exit(99);
    }
}



QString TarArch::getCompressor()
{
    QString extension = m_filename.right( m_filename.length() -
                                       m_filename.findRev('.') );

  if( extension == ".tgz" || extension == ".gz" )
    return QString( "gzip" );

  if( extension == ".bz")
    return QString( "bzip" );

  if( extension == ".Z" || extension == ".taz" )
    return QString( "compress" );

  if( extension == ".bz2")
    return QString( "bzip2" );

  if( extension == ".lzo" || extension == ".tzo" )
    return QString( "lzop" );

  return QString::null;
}

QString TarArch::getUnCompressorByExtension()
{
    QString extension = m_filename.right( m_filename.length() - m_filename.findRev('.') );

  if( extension == ".tgz" || extension == ".gz" )
    return QString( "gunzip" );

  if( extension == ".bz")
    return QString( "bunzip" );

  if( extension == ".Z" || extension == ".taz" )
    return QString( "uncompress" );

  if( extension == ".bz2")
    return QString( "bunzip2" );

  if( extension == ".lzo" || extension == ".tzo" )
    return QString( "lzop" );

  return QString::null;
}

QString TarArch::getUnCompressor()
{

    QString fileType = KMimeMagic::self()->findFileType( m_filename )->mimeType();

    if ( fileType == "application/x-compress" )
        return QString( "uncompress" );

    if ( fileType == "application/x-gzip" )
        return QString( "gunzip" );

    if ( fileType == "application/x-bzip2" )
        return QString( "bunzip2" );

    if( fileType == "application/x-zoo" )
        return QString( "lzop" );

    return getUnCompressorByExtension();
}

void TarArch::open()
{
  kdDebug(1601) << "+TarArch::open" << endl;
  unlink( QFile::encodeName(tmpfile) ); // just to make sure
  setHeaders();

  // might as well plunk the output of tar -tvf in the shell output window...
  KProcess *kp = new KProcess;

  *kp << m_archiver_program;

  if (compressed)
    *kp << "--use-compress-program="+getUnCompressor() ;

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
      KMessageBox::error( 0, i18n("Couldn't start a subprocess.") );
//      emit sigOpen(this, false, QString::null, 0 );
    }

  // We list afterwards because we want the signals at the end
  // This unconfuses Extract Here somewhat
  KTarGz *tarptr;
  bool failed = false;

  if (!compressed ||
      getUnCompressor() == QString("gunzip")
     || getUnCompressor() == QString("bunzip2"))
    {
      tarptr = new KTarGz(m_filename);
    }
  else
    {
      createTmp();
      while (compressed && createTmpInProgress)
        qApp->processEvents(); // wait for temp to be created;
      tarptr = new KTarGz(tmpfile);
    }

  failed = !tarptr->open(IO_ReadOnly);
  if(failed && (getUnCompressor() == QString("gunzip")
                || getUnCompressor() == QString("bunzip2")))
    {
      delete tarptr;
      createTmp();
      while (compressed && createTmpInProgress)
        qApp->processEvents(); // wait for temp to be created;
      tarptr = new KTarGz(tmpfile);
      failed = !tarptr->open(IO_ReadOnly);
    }

  if(failed)
      emit sigOpen(this, false, QString::null, 0 );
  else
    {
      processDir(tarptr->directory(), "");
      // because we aren't using the KProcess method, we have to emit this
      // ourselves.
      emit sigOpen(this, true, m_filename,
                    Arch::Extract | Arch::Delete | Arch::Add | Arch::View );
    }
  delete tarptr;


  kdDebug(1601) << "-TarArch::open" << endl;
}

void TarArch::slotListingDone(KProcess *_kp)
{
  delete _kp;
}

void TarArch::processDir(const KTarDirectory *tardir, const QString & root)
  // process a KTarDirectory. Called recursively for directories within
  // directories, etc. Prepends to filename root, for relative pathnames.
{
  kdDebug(1601) << "+TarArch::processDir" << endl;
  QStringList list = tardir->entries();
  for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it )
    {
      const KTarEntry* tarEntry = tardir->entry((*it));
      if (tarEntry == NULL)
        return;
      QStringList col_list;
      QString name;
      if (root.isEmpty() || root.isNull())
        name = tarEntry->name();
      else
        name = root + '/' + tarEntry->name();
      col_list.append( name );
      QString perms = makeAccessString(tarEntry->permissions());
      if (!tarEntry->isFile())
        perms = "d" + perms;
      else if (!tarEntry->symlink().isEmpty())
        perms = "l" + perms;
      else
        perms = "-" + perms;
      col_list.append(perms);
      QString usergroup = tarEntry->user();
      usergroup += '/';
      usergroup += tarEntry->group();
      col_list.append( usergroup );
      QString strSize = "0";
      if (tarEntry->isFile())
        {
          strSize.sprintf("%d", ((KTarFile *)tarEntry)->size());
        }
      col_list.append(strSize);
      QString timestamp = makeTimeStamp(tarEntry->datetime());
      col_list.append(timestamp);
      col_list.append(tarEntry->symlink());
      m_gui->listingAdd(&col_list); // send the entry to the GUI

      // if it isn't a file, it's a directory - process it.
      // remember that name is root + / + the name of the directory
      if (!tarEntry->isFile())
        processDir( (KTarDirectory *)tarEntry, name);
    }
  kdDebug(1601) << "-TarArch::processDir" << endl;
}

void TarArch::create()
{
  kdDebug(1601) << "+TarArch::createArch" << endl;

  emit sigCreate(this, true, m_filename,
                 Arch::Extract | Arch::Delete | Arch::Add
                  | Arch::View);
  kdDebug(1601) << "-TarArch::createArch" << endl;
}

void TarArch::setHeaders()
{
  kdDebug(1601) << "+TarArch::setHeaders" << endl;
  QStringList list;

  list.append(FILENAME_STRING);
  list.append(PERMISSION_STRING);
  list.append(OWNER_GROUP_STRING);
  list.append(SIZE_STRING);
  list.append(TIMESTAMP_STRING);
  list.append(LINK_STRING);

  // which columns to align right
  int *alignRightCols = new int[2];
  alignRightCols[0] = 1;
  alignRightCols[1] = 3;

  m_gui->setHeaders(&list, alignRightCols, 2);
  delete [] alignRightCols;

  kdDebug(1601) << "-TarArch::setHeaders" << endl;
}

void TarArch::createTmp()
{
  kdDebug(1601) << "+TarArch::createTmp" << endl;
  if (compressed)
    {
      struct stat statbuffer;
      if (stat(QFile::encodeName(tmpfile), &statbuffer) == -1)
        {
          // the tmpfile does not yet exist, so we create it.
          createTmpInProgress = true;
          fd = fopen( tmpfile.local8Bit(), "w" );

          KProcess *kp = new KProcess;
          kp->clearArguments();
          QString strUncompressor = getUnCompressor();
          kdDebug(1601) << "Uncompressor is " << strUncompressor << endl;
          *kp << strUncompressor;
          if (strUncompressor == "lzop")
            {
              *kp << "-d" ;
            }
          *kp << "-c" << m_filename.local8Bit();

          connect(kp, SIGNAL(processExited(KProcess *)),
                  this, SLOT(createTmpFinished(KProcess *)));
          connect(kp, SIGNAL(receivedStdout(KProcess*, char*, int)),
                  this, SLOT(createTmpProgress( KProcess *, char *, int )));
          connect( kp, SIGNAL(receivedStderr(KProcess*, char*, int)),
                   this, SLOT(slotReceivedOutput(KProcess*, char*, int)));

          if (kp->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false)
            {
              KMessageBox::error(0, i18n("I can't fork a decompressor"));
            }
        }
      else
        {
          kdDebug(1601) << "Temp tar already there..." << endl;
        }
    }
  kdDebug(1601) << "-TarArch::createTmp" << endl;
}

void TarArch::createTmpProgress( KProcess *, char *_buffer, int _bufflen )
{
  // we're trying to capture the output of a command like this
  //    gunzip -c myarch.tar.gz
  // and put the output into tmpfile.

  int size;
  size = fwrite(_buffer, 1, _bufflen, fd);
  if (size != _bufflen)
    {
      KMessageBox::error(0, i18n("Trouble writing to the tempfile..."));
      exit(99);
    }
}

static QDateTime getMTime(const QString & entry)
{
  // I have something like: 1999-10-04 11:04:44
  int year, month, day, hour, min, seconds;
  // HPB: should be okay 'cause the date format will not be localized
  sscanf( entry.ascii(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour,
          &min, &seconds);

  QDate theDate(year, month, day);
  QTime theTime(hour, min, seconds);
  return (QDateTime(theDate, theTime));
}


void TarArch::deleteOldFiles(QStringList *urls, bool bAddOnlyNew)
  // because tar is broken. Used when appending: see addFile.
{
  struct stat statbuffer;
  QStringList list;
  QString str;

  int col = m_gui->getCol(TIMESTAMP_STRING);

  QStringList::ConstIterator iter;
  for (iter = urls->begin(); iter != urls->end(); ++iter )
  {
    QString filename;
    str = *iter;
    if (str.left(5) == "file:")
      // get rid of "file:" part of url
      filename = str.right(str.length()-5);
    str = str.right(str.length()-8); // get rid of leading /
    if (!m_settings->getaddPath())
      str = str.right(str.length()-str.findRev('/')-1);
    if (bAddOnlyNew)
    {
      // compare timestamps. If the file to be added is newer, delete the
      // old. Otherwise we aren't adding it anyway, so we can go on to the next
      // file with a "continue".

      // find the file entry in the archive listing
      QString entryTimeStamp = m_gui->getColData(str, col);
      if (entryTimeStamp.isNull())
        continue;  // it isn't in there, so skip it.
      stat(QFile::encodeName(filename), &statbuffer);
      time_t the_mtime = statbuffer.st_mtime;
      struct tm *convertStruct = localtime(&the_mtime);
      QDateTime addFileMTime(QDate(convertStruct->tm_year,
                                   convertStruct->tm_mon + 1,
                                   convertStruct->tm_mday),
                             QTime(convertStruct->tm_hour,
                                   convertStruct->tm_min,
                                   convertStruct->tm_sec));
      QDateTime oldFileMTime = getMTime(entryTimeStamp);

      kdDebug(1601) << "Old file: " << oldFileMTime.date().year() << "-" <<
        oldFileMTime.date().month() << "-" << oldFileMTime.date().day() <<
        " " << oldFileMTime.time().hour() << ":" <<
        oldFileMTime.time().minute() << ":" << oldFileMTime.time().second() <<
        endl;
      kdDebug(1601) << "New file: " << addFileMTime.date().year()  << "-" <<
        addFileMTime.date().month()  << "-" << addFileMTime.date().day() <<
        " " << addFileMTime.time().hour()  << ":" <<
        addFileMTime.time().minute() << ":" << addFileMTime.time().second() <<
        endl;

      if (oldFileMTime >= addFileMTime)
      {
        fprintf(stderr, "Old time is newer or same\n");
        continue; // don't add this file to the list to be deleted.
      }
    }
    list.append(str);

    kdDebug() << "To delete: " << str << endl;
  }
  remove(&list);
}


void TarArch::addFile( QStringList* urls )
{
  kdDebug(1601) << "+TarArch::addFile" << endl;
  QString file, url, tmp;

  // tar is broken. If you add a file that's already there, it gives you
  // two entries for that name, whether you --append or --update. If you
  // extract by name, it will give you
  // the first one. If you extract all, the second one will overwrite the
  // first. So we'll first delete all the old files matching the names of
  // those in urls.
  m_bNotifyWhenDeleteFails = false;
  deleteOldFiles(urls, m_settings->getAddReplaceOnlyWithNewer());
  while (deleteInProgress)
    qApp->processEvents(); // wait for deletion
  m_bNotifyWhenDeleteFails = true;

  createTmp();
  while (compressed && createTmpInProgress)
    qApp->processEvents(); // wait for temp to be created;

  url = urls->first();
  file = KURL(url).path(-1); // remove trailing slash

  KProcess *kp = new KProcess;
  kp->clearArguments();
  *kp << m_archiver_program.local8Bit();

  if( m_settings->getAddReplaceOnlyWithNewer())
    *kp << "uvf";
  else
    *kp << "rvf";

  if (compressed)
    *kp << tmpfile.local8Bit();
  else
    *kp << m_filename;

  if (m_settings->getTarUseAbsPathnames())
    *kp << "-P";

  QString base;

  if( !m_settings->getaddPath() )
    {
      int pos;
      pos = file.findRev( '/', -1, FALSE );
      base = file.left( ++pos );
      kdDebug(1601) << "base is " << base << endl;
      //                pos++;
      tmp = file.right( file.length()-pos );
      file = tmp;
      chdir( base.local8Bit() );
    }
  QStringList::Iterator it=urls->begin();
  while(1)
    {
      int pos;
      *kp << file.local8Bit();
      it++;
      url = *it;

      if( url.isNull() )
        break;
      file = KURL(url).path(-1); // remove trailing slash
      pos = file.findRev( '/', -1, FALSE );
      pos++;
      tmp = file.right( file.length()-pos );
      file = tmp;
    }

  // debugging info
  QString strTemp;
  const QStrList *ptr = kp->args();
  QStrList list(*ptr); // copied because of const probs
  for ( strTemp=list.first(); strTemp != 0; strTemp=list.next() )
    {
      kdDebug(1601) << strTemp << " " << endl;
    }

  connect( kp, SIGNAL(receivedStdout(KProcess*, char*, int)),
           this, SLOT(slotReceivedOutput(KProcess*, char*, int)));
  connect( kp, SIGNAL(receivedStderr(KProcess*, char*, int)),
           this, SLOT(slotReceivedOutput(KProcess*, char*, int)));

  connect( kp, SIGNAL(processExited(KProcess*)), this,
           SLOT(slotAddFinished(KProcess*)));

  kdDebug(1601) << "Busy loop... waiting for temp tar to be created" << endl;
  while (compressed && createTmpInProgress)
    qApp->processEvents(); // wait for temp to be created;

  if (kp->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false)
    {
      KMessageBox::error( 0, i18n("Couldn't start a subprocess.") );
      emit sigAdd(false);
    }

#if 0
  if( m_settings->getaddPath() )
    file.remove( 0, 1 );  // Get rid of leading /
#endif

  kdDebug(1601) << "-TarArch::addFile" << endl;
}

void TarArch::slotAddFinished(KProcess *_kp)
{
  kdDebug(1601) << "+TarArch::slotAddFinished" << endl;

  disconnect( _kp, SIGNAL(processExited(KProcess*)), this,
              SLOT(slotAddFinished(KProcess*)));
  if (compressed)
    {
      updateArch();
      while (updateInProgress)
        qApp->processEvents(); // wait for update;
    }
  Arch::slotAddExited(_kp); // this will delete _kp
  kdDebug(1601) << "-TarArch::slotAddFinished" << endl;
}

void TarArch::unarchFile(QStringList * _fileList, const QString & _destDir,
                         bool viewFriendly)
{
  kdDebug(1601) << "+TarArch::unarchFile" << endl;
  QString dest;

  if (_destDir.isEmpty() || _destDir.isNull())
    dest = m_settings->getExtractDir();
  else dest = _destDir;

  QString tmp;

  KProcess *kp = new KProcess;
  kp->clearArguments();

  *kp << m_archiver_program.local8Bit();
  if (compressed)
    *kp << "--use-compress-program="+getUnCompressor() ;

  QString options = "-x";
  if (!m_settings->getExtractOverwrite())
    options += "k";
  if (m_settings->getTarPreservePerms())
    options += "p";
  options += "f";

  kdDebug(1601) << "Options were: " << options.local8Bit() << endl;
  *kp << options.local8Bit() << m_filename.local8Bit() << "-C" << dest;

  // if the list is empty, no filenames go on the command line,
  // and we then extract everything in the archive.
  if (_fileList)
    {
      for ( QStringList::Iterator it = _fileList->begin();
            it != _fileList->end(); ++it )
        {
          *kp << (*it).local8Bit();/*.latin1() ;*/
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
      KMessageBox::error( 0, i18n("Couldn't start a subprocess.") );
      emit sigExtract(false);
    }

  kdDebug(1601) << "+TarArch::unarchFile" << endl;
}

void TarArch::remove(QStringList *list)
{
  kdDebug(1601) << "+Tar::remove" << endl;
  deleteInProgress = true;
  QString name, tmp;

  createTmp();
  while (compressed && createTmpInProgress)
    qApp->processEvents(); // wait for temp to be created;

  KProcess *kp = new KProcess;
  kp->clearArguments();
  *kp << m_archiver_program.local8Bit() << "--delete" << "-f" ;
  if (compressed)
    *kp << tmpfile.local8Bit();
  else
    *kp << m_filename.local8Bit();

  for ( QStringList::Iterator it = list->begin(); it != list->end(); ++it )
    {
      kdDebug(1601) << *it << endl;
      *kp << *it;
    }

  connect( kp, SIGNAL(receivedStdout(KProcess*, char*, int)),
           this, SLOT(slotReceivedOutput(KProcess*, char*, int)));
  connect( kp, SIGNAL(receivedStderr(KProcess*, char*, int)),
           this, SLOT(slotReceivedOutput(KProcess*, char*, int)));

  connect( kp, SIGNAL(processExited(KProcess*)), this,
           SLOT(slotDeleteExited(KProcess*)));

  if (kp->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false)
    {
      KMessageBox::error( 0, i18n("Couldn't start a subprocess.") );
      emit sigDelete(false);
    }

  if (compressed)
    updateArch();

  kdDebug(1601) << "-Tar::remove" << endl;
}

void TarArch::slotDeleteExited(KProcess *_kp)
{
  deleteInProgress = false;
  Arch::slotDeleteExited(_kp);
}

void TarArch::addDir(const QString & _dirName)
{
  QStringList list;
  list.append(_dirName);
  addFile(&list);
}

void TarArch::openFinished( KProcess * )
{
  // do nothing
  // turn off busy light (when someone makes one)
  kdDebug(1601) << "Open finshed" << endl;
}

void TarArch::createTmpFinished( KProcess *_kp )
{
  kdDebug(1601) << "+TarArch::createTmpFinished" << endl;

  createTmpInProgress = false;
  fclose(fd);
  delete _kp;
  _kp = NULL;

  // turn off busy light (when someone makes one)

  kdDebug(1601) << "-TarArch::createTmpFinished" << endl;
}

void TarArch::updateFinished( KProcess *_kp )
{
  kdDebug(1601) << "+TarArch::updateFinished" << endl;
  fclose(fd);
  updateInProgress = false;
  delete _kp;
  _kp = NULL;

  kdDebug(1601) << "-TarArch::updateFinished" << endl;

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

static QString makeTimeStamp(const QDateTime & dt)
{
  // make sortable timestamp, like the output of tar -tvf
  // but with seconds.

  QString timestamp;
  QDate d = dt.date();
  QTime t = dt.time();

  timestamp.sprintf("%d-%02d-%02d %s",
                    d.year(), d.month(), d.day(),
                    t.toString().utf8().data());
  return timestamp;
}

#include "tar.moc"
