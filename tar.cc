/*

 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org
 1999-2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)

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

// note from Emily on future development (not yet implemented!! Soon.)
// When maintaining tar files with ark, the user should be
// aware that these options have been improved (IMHO). When you append a file
// to a tarchive, tar does not check if the file exists already, and just
// tacks the new one on the end. ark deletes the old one.
// When you update a file that exists in a tarchive, it does check if
// it exists, but once again, it creates a duplicate at the end (only if
// the file is newer though). ark deletes the old one in this case as well.
//
// Basically, tar files are great for creating and extracting, but
// not for maintaining. The original purpose of a tar was of course,
// for tape backups, so this is not so surprising!
//

#include <kurl.h>
// Unsorted in qdir.h is used, but in some of the headers
// below it's defined, too. So I brought kurl.h to the top.
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

// Qt includes
#include <qregexp.h>

// KDE includes
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

// ark includes
#include "viewer.h"
#include "extractdlg.h"
#include "tar.h"
#include "tar.moc"

static char *makeAccessString(mode_t mode);
static QString makeTimeStamp(const QDateTime & dt);

TarArch::TarArch( ArkSettings *_settings, Viewer *_gui,
		  const QString & _filename)
  : Arch(_settings, _gui, _filename), createTmpInProgress(false),
    updateInProgress(false), fd(NULL)
{
  kDebugInfo(1601, "+TarArch::TarArch");
  _settings->readTarProperties();
  if (_filename.right(4) == ".tar")
    {
      compressed = false;
    }
  else
    {
      compressed = true;
      QString tmpdir;
      tmpdir.sprintf("/tmp/ark.%d", getpid());

      QString base = m_filename.right(m_filename.length()- 1 -
				     m_filename.findRev("/"));
      base = base.left(base.findRev("."));
      
      // build the temp file name
      tmpfile.sprintf("%s/temp_tar_%s.%d", (const char *)tmpdir,
		      (const char *)base, getpid());
      kDebugInfo(1601, "Tmpfile will be %s\n",
		 (const char *)tmpfile.local8Bit());
    }
  kDebugInfo( 1601, "-TarArch::TarArch");
}

TarArch::~TarArch()
{
  unlink((const char *)tmpfile);
}

int TarArch::getEditFlag()
{
  return Arch::Extract;
}

int TarArch::updateArch()
{
  kDebugInfo(1601, "+TarArch::updateArch");
  if (compressed)
    {
      updateInProgress = true;
      fd = fopen( m_filename.local8Bit(), "w" );

      KProcess *kp = new KProcess;
      kp->clearArguments();
      *kp << getCompressor() << "-c" << tmpfile.local8Bit();

      connect(kp, SIGNAL(receivedStdout(KProcess*, char*, int)),
	      this, SLOT(updateProgress( KProcess *, char *, int )));
      connect( kp, SIGNAL(receivedStderr(KProcess*, char*, int)),
	       this, SLOT(slotReceivedOutput(KProcess*, char*, int)));

      connect(kp, SIGNAL(processExited(KProcess *)),
	       this, SLOT(updateFinished(KProcess *)) );
      if (kp->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false)
	{
	  KMessageBox::error(0, i18n("Trouble writing to the archive..."));
	}
    }
  kDebugInfo(1601, "-TarArch::updateArch");
  return SUCCESS;
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
  QString extension = m_filename.right(m_filename.length() -
				       m_filename.findRev('.') );
  kDebugInfo(1601, "Extension: %s", (const char *)extension);

  if( extension == ".tgz" || extension == ".gz" ) 
    return QString( "gzip" );
  if( extension == ".bz" )
    return QString( "bzip" );
  if( extension == ".Z" || extension == ".taz" )
    return QString( "compress" );
  if( extension == ".bz2" )
    return QString( "bzip2" );
  if( extension == ".lzo" || extension == ".tzo" )
    return QString( "lzop" );
  return QString::null;
}

QString TarArch::getUnCompressor() 
{
  QString extension = m_filename.right(m_filename.length() -
				       m_filename.findRev('.'));
  kDebugInfo(1601, "Extension: %s", (const char *)extension);
  if( extension == ".tgz" || extension == ".gz" ) 
    return QString( "gunzip" );
  if( extension == ".bz" )
    return QString( "bunzip" );
  if( extension == ".Z" || extension == ".taz" )
    return QString( "uncompress" );
  if( extension == ".bz2" )
    return QString( "bunzip2" );
  if( extension == ".lzo" || extension == ".tzo" )
    return QString( "lzop" );
  return QString::null;
}

void TarArch::open()
{
  kDebugInfo(1601, "+TarArch::open");
  unlink((const char *)tmpfile); // just to make sure
  setHeaders();
  KTarGz *tarptr;

  if (!compressed || 
      getUnCompressor() == QString("gunzip"))
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

  if (! tarptr->open(IO_ReadOnly))
    {
      emit sigOpen(this, false, QString::null, 0 );
    }
  else
    {
      processDir(tarptr->directory(), "");
      // because we aren't using the KProcess method, we have to emit this
      // ourselves.
      emit sigOpen(this, true, m_filename,
		    Arch::Extract | Arch::Delete | Arch::Add | Arch::View );
    }
  delete tarptr;

  // might as well plunk the output of tar -tvf in the shell output window...
  KProcess *kp = new KProcess;
  QString tar_exe = m_settings->getTarCommand();
  *kp << tar_exe.local8Bit() << "--use-compress-program="+getUnCompressor() ;
  *kp << "-tvf" << m_filename.local8Bit();
  connect(kp, SIGNAL(processExited(KProcess *)),
	  this, SLOT(slotListingDone(KProcess *)));
  connect(kp, SIGNAL(receivedStdout(KProcess*, char*, int)),
	  this, SLOT(slotReceivedOutput( KProcess *, char *, int )));
  connect( kp, SIGNAL(receivedStderr(KProcess*, char*, int)),
	   this, SLOT(slotReceivedOutput(KProcess*, char*, int)));

  if (kp->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false)
    {
      KMessageBox::error(0, i18n("Error in trying to list the contents of the archive."));
    }

  kDebugInfo(1601, "-TarArch::open");
}

void TarArch::slotListingDone(KProcess *_kp)
{
  delete _kp;
}

void TarArch::processDir(const KTarDirectory *tardir, const QString & root)
  // process a KTarDirectory. Called recursively for directories within
  // directories, etc. Prepends to filename root, for relative pathnames.
{
  kDebugInfo(1601, "+TarArch::processDir");
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
	name = root + "/" + tarEntry->name();
      col_list.append(QString::fromLocal8Bit(name));
      QString perms = makeAccessString(tarEntry->permissions());
      if (!tarEntry->isFile())
	perms = "d" + perms;
      else if (!tarEntry->symlink().isEmpty())
	perms = "l" + perms;
      else
	perms = "-" + perms;
      col_list.append(QString::fromLocal8Bit(perms));
      QString usergroup = tarEntry->user();
      usergroup += '/';
      usergroup += tarEntry->group();
      col_list.append(QString::fromLocal8Bit(usergroup));
      QString strSize = "0";
      if (tarEntry->isFile())
	{
	  strSize.sprintf("%d", ((KTarFile *)tarEntry)->size());
	}
      col_list.append(QString::fromLocal8Bit(strSize));
      QString timestamp = makeTimeStamp(tarEntry->datetime());
      col_list.append(QString::fromLocal8Bit(timestamp));
      col_list.append(QString::fromLocal8Bit(tarEntry->symlink()));
      m_gui->add(&col_list); // send the entry to the GUI

      // if it isn't a file, it's a directory - process it.
      // remember that name is root + / + the name of the directory
      if (!tarEntry->isFile())
	processDir( (KTarDirectory *)tarEntry, name);
    }
  kDebugInfo(1601, "-TarArch::processDir");
}                                                                           

void TarArch::create()
{
  kDebugInfo(1601, "+TarArch::createArch");

  emit sigCreate(this, true, m_filename,
		 Arch::Extract | Arch::Delete | Arch::Add 
		  | Arch::View);
  kDebugInfo(1601, "-TarArch::createArch");
}

void TarArch::setHeaders()
{
  kDebugInfo(1601, "+TarArch::setHeaders");
  QStringList list;

  list.append(i18n(" Filename ") );
  list.append(i18n(" Permissions ") );
  list.append(i18n(" Owner/Group ") );
  list.append(i18n(" Size ") );
  list.append(i18n(" TimeStamp ") );
  list.append(i18n(" Link "));

  // which columns to align right
  int *alignRightCols = new int[2];
  alignRightCols[0] = 1;
  alignRightCols[1] = 3;
  
  m_gui->setHeaders(&list, alignRightCols, 2);
  delete [] alignRightCols;

  kDebugInfo(1601, "-TarArch::setHeaders");
}

void TarArch::createTmp()
{
  kDebugInfo(1601, "+TarArch::createTmp");
  if (compressed)
    {
      struct stat statbuffer;
      if (stat((const char *)tmpfile, &statbuffer) == -1)
	{
	  // the tmpfile does not yet exist, so we create it.
	  createTmpInProgress = true;
	  fd = fopen( tmpfile.local8Bit(), "w" );

	  KProcess *kp = new KProcess;
	  kp->clearArguments();
	  QString strUncompressor = getUnCompressor();
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
	  kDebugInfo(1601, "Temp tar already there...");
	}
    }
  kDebugInfo(1601, "-TarArch::createTmp");
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

void TarArch::addFile( QStringList* urls )
{
  kDebugInfo(1601, "+TarArch::addFile");
  QString file, url, tmp;
  QString tar_exe = m_settings->getTarCommand();
		
  createTmp();
  while (compressed && createTmpInProgress)
    qApp->processEvents(); // wait for temp to be created;

  url = urls->first();
  file = KURL(url).path(-1); // remove trailing slash

  KProcess *kp = new KProcess;
  kp->clearArguments();
  *kp << tar_exe.local8Bit();
	
  if( m_settings->getReplaceOnlyNew())
    *kp << "uvf";
  else
    *kp << "rvf";
  if (compressed)
    *kp << tmpfile.local8Bit();
  else
    *kp << m_filename;
	
  QString base;

  if( !m_settings->getaddPath() )
    {
      int pos;
      pos = file.findRev( '/', -1, FALSE );
      base = file.left( ++pos );
      kDebugInfo(1601, "base is %s", (const char *)base);
      //		pos++;
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
      kDebugInfo(1601, "%s ", (const char *)strTemp);
    }

  connect( kp, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(slotReceivedOutput(KProcess*, char*, int)));
  connect( kp, SIGNAL(receivedStderr(KProcess*, char*, int)),
	   this, SLOT(slotReceivedOutput(KProcess*, char*, int)));

  connect( kp, SIGNAL(processExited(KProcess*)), this,
	   SLOT(slotAddFinished(KProcess*)));

  kDebugInfo(1601, "Busy loop... waiting for temp tar to be created");
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

  kDebugInfo(1601, "-TarArch::addFile");
}

void TarArch::slotAddFinished(KProcess *_kp)
{
  kDebugInfo(1601, "+TarArch::slotAddFinished");

  disconnect( _kp, SIGNAL(processExited(KProcess*)), this,
	      SLOT(slotAddFinished(KProcess*)));
  if (compressed)
    {
      updateArch();
      while (updateInProgress)
	qApp->processEvents(); // wait for update;
    }
  Arch::slotAddExited(_kp); // this will delete _kp
  kDebugInfo(1601, "-TarArch::slotAddFinished");
}

void TarArch::unarchFile( QStringList * _fileList, const QString & _destDir)
{
  kDebugInfo( 1601, "+TarArch::unarchFile");
  QString dest;

  if (_destDir.isEmpty() || _destDir.isNull())
    dest = m_settings->getExtractDir();
  else dest = _destDir;

  QString tmp;
  QString tar_exe = m_settings->getTarCommand();	
	
  KProcess *kp = new KProcess;
  kp->clearArguments();
  
  *kp << tar_exe.local8Bit() << "--use-compress-program="+getUnCompressor() ;
  if (m_settings->getTarPreservePerms())
    *kp << "-xvpf";
  else
    *kp << "-xvf";

  *kp << m_filename.local8Bit() << "-C" << dest;	

  // if the list is empty, no filenames go on the command line,
  // and we then extract everything in the archive.
  if (_fileList)
    {
      for ( QStringList::Iterator it = _fileList->begin();
	    it != _fileList->end(); ++it ) 
	{
	  *kp << (*it).latin1() ;
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

  kDebugInfo(1601, "+TarArch::unarchFile");
}

void TarArch::remove(QStringList *list)
{
  kDebugInfo( 1601, "+Tar::deleteFiles");

  QString name, tmp;
  QString tar_exe = m_settings->getTarCommand();	
  
  createTmp();
  while (compressed && createTmpInProgress)
    qApp->processEvents(); // wait for temp to be created;

  KProcess *kp = new KProcess;	
  kp->clearArguments();
  *kp << tar_exe.local8Bit() << "--delete" << "-f" ;
  if (compressed)
    *kp << tmpfile.local8Bit();
  else
    *kp << m_filename.local8Bit();

  for ( QStringList::Iterator it = list->begin(); it != list->end(); ++it )  
    {
      kDebugInfo(1601, "%s", (const char *)*it);
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

  kDebugInfo( 1601, "-Tar::deleteFiles");
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
  kDebugInfo(1601, "Open finshed");
}

void TarArch::createTmpFinished( KProcess *_kp )
{
  kDebugInfo(1601, "+TarArch::createTmpFinished");

  createTmpInProgress = false;
  fclose(fd);
  delete _kp;
  _kp = NULL;

  // turn off busy light (when someone makes one)

  kDebugInfo(1601, "-TarArch::createTmpFinished");
}

void TarArch::updateFinished( KProcess *_kp )
{
  kDebugInfo(1601, "+TarArch::updateFinished");
  fclose(fd);
  updateInProgress = false;
  delete _kp;
  _kp = NULL;

  kDebugInfo(1601, "-TarArch::updateFinished");

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
		    (const char *)t.toString());
  return timestamp;
}
