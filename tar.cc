/*

 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org
 1999-2000: Corel Corporation (Emily Ezust, emilye@corel.com)

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
  : Arch(_settings, _gui, _filename)
{
  kDebugInfo(1601, "+TarArch::TarArch");
  stdout_buf = NULL;
  if (_filename.right(4) == ".tar")
    {
      compressed = false;
    }
  else
    {
      compressed = true;
      tmpdir.sprintf("/tmp/ark.%d", getpid());
    }
  kDebugInfo( 1601, "-TarArch::TarArch");
}

TarArch::~TarArch()
{
}

int TarArch::getEditFlag()
{
  return Arch::Extract;
}

int TarArch::updateArch()
{
  if (compressed)
    {
      // copy the tmpfile to a junk name, compress, and move the file
      // to where m_filename is. Then rename junk name back to tmpfile.

      QString command;
      command.sprintf("cp '%s' %s/junkname",
		      (const char *)tmpfile, (const char *)tmpdir);

      kDebugInfo(1601, "Command is %s", (const char *)command);
      system((const char *)command);

      m_kp->clearArguments();
      *m_kp << getCompressor() << tmpfile.local8Bit();
      connect( m_kp, SIGNAL(processExited(KProcess *)),
	       this, SLOT(openFinished(KProcess *)) );
      if( m_kp->start( KProcess::Block, KProcess::Stdout ) == FALSE )
	{
	  KMessageBox::error(0, "Can't fork a compressor.");
	  return FAILURE;
	}
      
      // temparch will be m_filename but with tmpdir as the path
      QString temparch = tmpdir + "/" + 
	m_filename.right(m_filename.length() - 1 - 
			m_filename.findRev("/"));
      kDebugInfo(1601, "Temparch is %s", (const char *)temparch);

      command.sprintf("mv '%s' '%s'",
		      (const char *)temparch, (const char *)m_filename);
      kDebugInfo(1601, "Command is %s", (const char *)command);
      system((const char *)command);

      // rename back
      command.sprintf("mv /tmp/ark.%d/junkname '%s'",
		      getpid(), (const char *)tmpfile);
      kDebugInfo(1601, "Command is %s", (const char *)command);
      system((const char *)command);
    }
  return SUCCESS;
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

  connect( m_kp, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(slotOpenDataStdout(KProcess*, char*, int)));

  kDebugInfo(1601, "Connected receivedStdout to slotOpenDataStdout");

  setHeaders();
  
  KTarGz *tarptr = new KTarGz(m_filename);

  if (! tarptr->open(IO_ReadOnly))
    {
      emit sigOpen( false, QString::null, 0 );
    }
  else
    {
      processDir(tarptr->directory(), "");
      // because we aren't using the KProcess method, we have to emit this
      // ourselves.
      emit sigOpen( true, m_filename,
		    Arch::Extract | Arch::Delete | Arch::Add | Arch::View );
    }
  delete tarptr;
  kDebugInfo(1601, "-TarArch::open");
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

void TarArch::slotOpenDataStdout(KProcess* _p, char* _data, int _length)
{
  char c = _data[_length];
  _data[_length] = '\0';

  m_settings->appendShellOutputData( _data );
  _data[_length] = c;

}

void TarArch::create()
{
  kDebugInfo(1601, "+TarArch::createArch");

  emit sigCreate( true, m_filename, Arch::Extract | Arch::Delete | Arch::Add 
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

static QString removeExtension(const QString &_filename)
{
  QString name;
  if (_filename.right(4) == ".tgz" || _filename.right(4) == ".taz" ||
      _filename.right(4) == ".tzo")
    {
      name = _filename.left(_filename.length() - 3);
      name += "tar";
   }
  else
    // filename already has "tar." with an additional extension
    {
      name = _filename.left(_filename.findRev("."));
    }
  return name;
}

void TarArch::createTmp()
{
  kDebugInfo(1601, "+TarArch::createTmp");
  if (compressed)
    {
      // copy the archive to the tmp directory unless it's there already
      if (tmpfile.isEmpty())
	{
	  // build the tmpfile name
	  tmpfile = tmpdir + "/" + m_filename.right(m_filename.length() - 1 - 
						    m_filename.findRev('/'));
	  kDebugInfo(1601, "Tmpfile is %s\n",
		     (const char *)tmpfile.local8Bit());
	  m_kp->clearArguments();
	  *m_kp << "cp" << m_filename.local8Bit() << tmpdir;
	  if (m_kp->start( KProcess::Block, KProcess::Stdout ) == FALSE )
	    {
	      KMessageBox::error(0, i18n("I can't copy the archive to a temporary directory"));
	    }
	  
	  
	  // uncompress the temporary file
	  m_kp->clearArguments();
	  QString strUncompressor = getUnCompressor();
	  *m_kp << strUncompressor;
	  *m_kp << tmpfile;
	  
	  // debugging info
	  QString strTemp;
	  const QStrList *ptr = m_kp->args();
	  QStrList list(*ptr); // copied because of const probs
	  for ( strTemp=list.first(); strTemp != 0; strTemp=list.next() )
	    {
	      kDebugInfo(1601, "%s ", (const char *)strTemp);
	    }
	  
	  connect(m_kp, SIGNAL(processExited(KProcess *)),
		  this, SLOT(createTmpFinished(KProcess *)));
	  
	  if( m_kp->start( KProcess::Block, KProcess::Stdout ) == FALSE )
	    {
	      KMessageBox::error(0, "Can't fork a decompressor");
	    }
	  tmpfile = removeExtension(tmpfile); // for next time we need it
	  kDebugInfo(1601, "Tmpfile is %s\n",
		     (const char *)tmpfile.local8Bit());
	}
      else
	{
	  kDebugInfo(1601, "Temp tar already there...");
	}
    }
  kDebugInfo(1601, "-TarArch::createTmp");
}

int TarArch::addFile( QStringList* urls )
{
  kDebugInfo(1601, "+TarArch::addFile");

  int retcode;
  QString file, url, tmp;
  QString tar_exe = m_settings->getTarCommand();
		
  createTmp();

  url = urls->first();
  file = KURL(url).path(-1); // remove trailing slash

  m_kp->clearArguments();
  *m_kp << tar_exe.local8Bit();
	
  if( m_settings->getonlyUpdate() )
    *m_kp << "uvf";
  else
    *m_kp << "rvf";
  if (compressed)
    *m_kp << tmpfile.local8Bit();
  else
    *m_kp << m_filename;
	
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
      *m_kp << file.local8Bit();
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
  const QStrList *ptr = m_kp->args();
  QStrList list(*ptr); // copied because of const probs
  for ( strTemp=list.first(); strTemp != 0; strTemp=list.next() )
    {
      kDebugInfo(1601, "%s ", (const char *)strTemp);
    }

#if 0
  connect( m_kp, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(slotOpenDataStdout(KProcess*, char*, int)));
#endif
  connect( m_kp, SIGNAL(processExited(KProcess *)), 
	   this, SLOT(addFinished(KProcess *)));

  if( m_kp->start( KProcess::Block, KProcess::Stdout ) == FALSE )
    {
      KMessageBox::error(0, "Can't start a kprocess.");
      return FAILURE;
    }
	
  if( m_settings->getaddPath() )
    file.remove( 0, 1 );  // Get rid of leading /

  retcode = updateArch();
  return retcode;
  kDebugInfo(1601, "-TarArch::addFile");
}

#if 0
void TarArch::extraction()
{
  QString dir, ex;

  ExtractDlg ld( ExtractDlg::All );
  int mask = setOptions( FALSE, FALSE, FALSE );
  ld.setMask( mask );
  if( ld.exec() )
    {
      dir = ld.getDest();
      if( dir.isEmpty() )
	return;
      QDir dest( dir );
      if( !dest.exists() ) {
	if( mkdir( dir.local8Bit(), S_IWRITE | S_IREAD | S_IEXEC ) ) {
				//arkWarning( i18n("Unable to create destination directory") );
	  return;
	}
      }
      setOptions( ld.doPreservePerms(), ld.doLowerCase(), ld.doOverwrite() );
      switch( ld.extractOp() ) {
      case ExtractDlg::All: {
	extractTo( dir );
	break;
      }
      }
    }
}	

void TarArch::extractTo( const QString & dir )
{
  kDebugInfo(1601, "+TarArch::extractTo");


  QString tar_exe = m_settings->getTarCommand();
		
  m_kp->clearArguments();
  *m_kp << tar_exe.local8Bit();
  *m_kp << "--use-compress-program="+getUnCompressor() 
	<<	"-xvf" << m_filename.local8Bit() << "-C" << dir;	

  connect( m_kp, SIGNAL(processExited(KProcess *)), 
	   this, SLOT(extractFinished(KProcess *)));
#if 0
  connect( m_kp, SIGNAL(receivedStdout(KProcess *, char *,int)),
	   this, SLOT(updateExtractProgress(KProcess *, char *, int)));
#endif

  if(m_kp->start( KProcess::NotifyOnExit, KProcess::Stdout ) == false)
    {
      KMessageBox::error(0,"Subprocess wouldn't start!");
      return;
    }
  kDebugInfo(1601, "-TarArch::extractTo");
}
#endif

/* untested */
QString TarArch::unarchFile( QStringList * _fileList, const QString & _destDir)
{
#if 0
  kDebugInfo(1601, "+TarArch::unarchFile");

  QString dest;

  if (_destDir.isEmpty() || destDir.isNull())
    QString dest = m_settings->getExtractDir();
  else
    dest = _destDir;



  int pos;
  QString tmp, name;
  QString fullname;
  QString tar_exe = m_settings->getTarCommand();	
	
  //updateArch();
	
  tmp = (*listing)[index];
  pos = tmp.findRev( '\t', -1, FALSE );
  pos++;
  name = tmp.right( tmp.length()-pos );

  //	archProcess.clearArguments();
  //	archProcess.setExecutable( tar_exe );
  //	archProcess << tar_exe;

  m_kp->clearArguments();
  *m_kp << tar_exe.local8Bit();
	
  if (perms)
    *m_kp << "--use-compress-program="+getUnCompressor() << "-xvpf";
  else
    *m_kp << "--use-compress-program="+getUnCompressor() << "-xvf";

  *m_kp << m_filename.local8Bit() << "-C" << dest.local8Bit()
	<< name.local8Bit();
  if (m_kp->start(KProcess::Block) == false )
    {
      KMessageBox::error(0,"Can't start a kprocess.");
    }
  fullname = dest + name;
  kDebugInfo(1601, "+TarArch::unarchFile");

  return fullname;
#endif
  return "";
}

void TarArch::remove(QStringList *list)
{
  kDebugInfo( 1601, "+Tar::deleteFiles");

  QString name, tmp;
  QString tar_exe = m_settings->getTarCommand();	
	
  createTmp();
	
  m_kp->clearArguments();
  *m_kp << tar_exe.local8Bit() << "--delete" << "-f" ;
  if (compressed)
    *m_kp << tmpfile.local8Bit();
  else
    *m_kp << m_filename.local8Bit();

  for ( QStringList::Iterator it = list->begin(); it != list->end(); ++it )  
    {
      kDebugInfo(1601, "%s", (const char *)*it);
      *m_kp << *it;
    }

  m_kp->start(KProcess::Block);
  updateArch();

  kDebugInfo( 1601, "-Tar::deleteFiles");
}

void TarArch::updateExtractProgress( KProcess *, char *buffer, int bufflen )
{
  // some kind of progress bar, or something?

  // I know it's ugly, but I want the names for future reference.
  // purpose of this is to turn off warnings?
  buffer = buffer, bufflen = bufflen;
}

void TarArch::inputPending( KProcess *, char *buffer, int bufflen )
{
#if 0
  char    columns[6][255];
  char    line[512];
  char    wline[512];
  char   *pos;
  char   *start;
  char   *mybuf;
  char   *tok;
  int     i;

  kDebugInfo(1601, ".");

  bufflen = bufflen;
    
  if( stdout_buf == NULL )
    stdout_buf = strdup( "" );

  mybuf = (char *) malloc( strlen(stdout_buf) + bufflen+1 );
  strcpy( mybuf, stdout_buf );
  strncpy( &mybuf[strlen(stdout_buf)], buffer, bufflen );
  mybuf[strlen(stdout_buf)+bufflen] = 0;

  start = pos = mybuf;
  while( (pos = strchr( start, '\n' )) != NULL )
    {
      /* the sscanf doesn't work with symbolic links, I'm not sure why */
      //       sscanf( start, " %[-drwxst] %[0-9.a-zA-Z/_] %[0-9] %17[a-zA-Z0-9:- ] %[^\n]",
      //              columns[0], columns[1], columns[2], columns[3],
      //               filename
      //             );

#if 0   // yech! GUI stuff! take it away!
      FileLVI *flvi = new FileLVI(destination_flw);
      strncpy( wline, start, pos-start );
      wline[pos-start] = 0;
      tok = strtok( wline, " " );
      strcpy( columns[0], tok );
      for( i=1; i<6; i++ )
	{
	  tok = strtok( NULL, " " );
	  strcpy( columns[i], tok );
	}
      strcat( columns[3], columns[4] );
      flvi->setText(0, QString::fromLocal8Bit(columns[5]));
      for(int i=0; i<4; i++)
	{
	  flvi->setText(i+1, QString::fromLocal8Bit(columns[i]));
	}
      destination_flw->insertItem(flvi);

      sprintf(line, "%s\t%s\t%s\t%s\t%s",
	      columns[0], columns[1], columns[2], columns[3],
	      columns[5]);
      listing->append( QString::fromLocal8Bit(line) );
      start = (pos+1);
#endif
    }
  free( stdout_buf );
  stdout_buf = strdup( start );
  free( mybuf );
#endif
}

int TarArch::addDir(const QString & _dirName)
{
  return FAILURE;  // not implemented yet
}

void TarArch::extractFinished( KProcess * )
{
  // turn off busy light (when someone makes one)
  kDebugInfo(1601, "Extract Finished");
}

void TarArch::openFinished( KProcess * )
{
  // do nothing
  // turn off busy light (when someone makes one)
  kDebugInfo(1601, "Open finshed");
}

void TarArch::createTmpFinished( KProcess * )
{
  // do nothing
  // turn off busy light (when someone makes one)
  kDebugInfo(1601, "Create Tmp finished");
}


// copy the working archive to the real archive
void TarArch::updateFinished( KProcess * )
{
#if 0
  FILE   *fd, fd2;
  char    buffer[4096];
  int     size = 0;

  // turn off busy light (when someone makes one)


  fd = fopen( tmpfile.local8Bit(), "r" );
  fd2 = fopen( m_filename, "w" );
		
  while( (size = fread( buffer, 1, 4096, fd )) )
    fwrite(buffer, size, 1, fd2);
  fclose(fd2);
#endif
    
  // is this necessary?
  //    if( !retcode )
  //    {
  //        listing->clear();
  //        openArch( m_filename );
  //    }


  kDebugInfo(1601, "Update finshed");
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
