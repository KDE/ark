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

// note from Emily:
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
#include <qmessagebox.h>

// KDE includes
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

// ark includes
#include "viewer.h"
#include "extractdlg.h"
#include "tar.h"
#include "tar.moc"

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
    }

  kDebugInfo( 1601, "-TarArch::TarArch");
}

TarArch::~TarArch()
{
  QString strCmd("rm -f " + tmpfile);
  updateArch();
}



int TarArch::getEditFlag()
{
  return Arch::Extract;
}

int TarArch::updateArch()
{
  if (!compressed)
    {
      compressed = TRUE;
      disconnect( m_kp, 0, 0, 0 );
      m_kp->clearArguments();
      *m_kp << getCompressor() << "-c" << tmpfile.local8Bit() << " > " 
	    << m_filename.local8Bit();
      connect( m_kp, SIGNAL(processExited(KProcess *)),
	       this, SLOT(openFinished(KProcess *)) );
      if( m_kp->start( KProcess::NotifyOnExit ) == FALSE )
	{
	  KMessageBox::error(0, "Can't fork a compressor.");
	  return -1;
	}
    }
  return 0;
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
  setHeaders();
  initOpen();

  if (m_kp->start( KProcess::NotifyOnExit, KProcess::Stdout ) == FALSE )
    {
      KMessageBox::error(0, "Can't fork a decompressor.");
      emit sigOpen( false, QString::null, 0 );
      return;
    }
  kDebugInfo(1601, "-TarArch::open");
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

  // which columns to align right
  int *alignRightCols = new int[2];
  alignRightCols[0] = 1;
  alignRightCols[1] = 3;
  
  m_gui->setHeaders(&list, alignRightCols, 2);
  delete [] alignRightCols;

  kDebugInfo(1601, "-TarArch::setHeaders");
}

void TarArch::initOpen()
{
  m_kp->clearArguments();
  QString tar_exe = m_settings->getTarCommand();

  if (tar_exe.isEmpty() || tar_exe.isNull())
    tar_exe = "tar";

  *m_kp << tar_exe;

  if (compressed)
    {
      *m_kp << "--use-compress-program="+getUnCompressor();
    }

  *m_kp << "-tvf" << m_filename.local8Bit();

  disconnect( m_kp, 0, 0, 0 );
  connect(m_kp, SIGNAL(processExited(KProcess *)), 
	  this, SLOT(openFinished(KProcess *)));
  connect(m_kp, SIGNAL(receivedStdout(KProcess *, char *,int)),
	  this, SLOT(inputPending(KProcess *, char *, int)));
}

void TarArch::processLine( char *_line )
{
  char columns[4][80];
  char filename[4096];
  
  sscanf(_line, " %[-drwxst] %[0-9.a-zA-Z/_-] %[0-9] %17[a-zA-Z0-9:- ] %[^\n]",
	 columns[0], columns[1], columns[2], columns[3],
	 filename);
 
  QStringList list;
  list.append(QString::fromLocal8Bit(filename));
  for (int i = 0; i < 7; ++i)
    {
      list.append(QString::fromLocal8Bit(columns[i]));
    }
  m_gui->add(&list); // send the entry to the GUI
}

void TarArch::slotOpenDataStdout(KProcess* _p, char* _data, int _length)
{
  kDebugInfo(1601, "+TarArch::slotOpenDataStdout");
  char c = _data[_length];
  _data[_length] = '\0';
	
  m_settings->appendShellOutputData( _data );

  kDebugInfo(1601, "-TarArch::slotOpenDataStdout");
  
}

/* untested */
void TarArch::createTmp()
{
  kDebugInfo(1601, "+TarArch::createTmp");
  if (compressed )
    {
      //		FILE* fd, *fd2;
      //		char buffer[4096];
      //		int size = 0;
      compressed = FALSE;

      m_kp->clearArguments();
      *m_kp << "gunzip" << "-c" << m_filename.local8Bit() << " > "
	    << tmpfile.local8Bit();
		
      disconnect( m_kp, 0, 0, 0 );
      connect(m_kp, SIGNAL(processExited(KProcess *)),
	      this, SLOT(createTmpFinished(KProcess *)));
		
      if( m_kp->start( KProcess::NotifyOnExit ) == FALSE )
	{
	  KMessageBox::error(0, "Can't fork a decompressor");
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
  *m_kp << tmpfile.local8Bit();
	
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

  if( m_kp->start( KProcess::NotifyOnExit, KProcess::Stdout ) == FALSE )
    {
      KMessageBox::error(0, "Can't start a kprocess.");
      return -1;
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

  disconnect( m_kp, 0, 0, 0 );
  connect( m_kp, SIGNAL(processExited(KProcess *)), 
	   this, SLOT(extractFinished(KProcess *)));
  connect( m_kp, SIGNAL(receivedStdout(KProcess *, char *,int)),
	   this, SLOT(updateExtractProgress(KProcess *, char *, int)));

  if(m_kp->start( KProcess::NotifyOnExit, KProcess::Stdout ) == false)
    {
      KMessageBox::error(0,"Subprocess wouldn't start!");
      return;
    }
  kDebugInfo(1601, "-TarArch::extractTo");
}
#endif

/* untested */
QString TarArch::unarchFile( QStringList * _fileList)
{
#if 0
  kDebugInfo(1601, "+TarArch::unarchFile");

  QString dest = m_settings->getExtractDir();

  int pos;
  QString tmp, name;
  QString fullname;
  QString tar_exe = m_settings->getTarCommand();	
	
  updateArch();
	
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

void TarArch::remove(QStringList *)
{
#if 0
  kDebugInfo( 1601, "+Tar::deleteFiles");

  QString name, tmp;
  QString tar_exe = m_settings->getTarCommand();	
	
  createTmp();
	
  m_kp->clearArguments();
  *m_kp << tar_exe.local8Bit() << "--delete" << "-f" << tmpfile.local8Bit() << patterns.local8Bit();
  m_kp->start(KProcess::Block);
	
  updateArch();

#endif
  kDebugInfo( 1601, "-Tar::deleteFiles");
}

void TarArch::updateExtractProgress( KProcess *, char *buffer, int bufflen )
{
  // some kind of progress bar, or something?

  // I know its ugly, but I wan't the names for future reference
  buffer = buffer, bufflen = bufflen;
}

void TarArch::inputPending( KProcess *, char *buffer, int bufflen )
{
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
#if 0
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

}

int TarArch::addDir(const QString & _dirName)
{
  return SUCCESS;
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
