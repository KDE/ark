/*

 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

 2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)

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

#include <iostream.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <string.h>

#include <qfile.h>

// KDE includes
#include <kurl.h>
#include <qstringlist.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

// ark includes
#include "rar.h"

// the generic viewer to which to send header and column info.
#include "viewer.h"

RarArch::RarArch( ArkSettings *_settings, Viewer *_gui,
		  const QString & _fileName )
  : Arch(_settings, _gui, _fileName ), m_linenumber(0)
{
  kdDebug(1601) << "RarArch constructor" << endl;
  m_archiver_program = "rar";
  m_unarchiver_program = "rar"; // some distributions of rar don't have unrar (bug #7112)
  verifyUtilityIsAvailable(m_archiver_program, m_unarchiver_program);
}

void RarArch::processLine( char *_line )
{
  // For each rar entry, this function is called exactly three times.
  // The first time, store the first line and return.
  // The second time, store the second line and return.
  // The third time, process the data in the first two lines and
  // send to the GUI. We ignore the third line since the data there
  // isn't really that important.

  ++m_linenumber;
  if (m_linenumber == 1)
    {
      m_line1 = QString::fromLocal8Bit(_line);
      return;
    }
  if (m_linenumber == 2)
    {
      m_line2 = QString::fromLocal8Bit(_line);
      return;
    }
  // if we made it here, we have all three lines.
  // Reset the line number.
  m_linenumber = 0;

  char columns[11][80];
  char filename[4096];
  sscanf(QFile::encodeName(m_line1), " %[^\n]", filename);
  sscanf((const char *)m_line2.ascii(), " %[0-9] %[0-9] %[0-9%] %2[0-9]-%2[0-9]-%2[0-9] %5[0-9:] %[drwxlst-] %[A-F0-9] %[A-Za-z0-9] %[0-9.]",
	 columns[0], columns[1], columns[2], columns[3],
	 columns[8], columns[9], columns[10],
	 columns[4], columns[5], columns[6],
	 columns[7]);

  // rearrange columns 3, 8, 9 so that the sort will work.
  // columns[3] is the day
  // columns[8] is the month
  // columns[9] is a 2-digit year. Ugh. Y2K junk here.
  
  QString year = Utils::fixYear(columns[9]);

  // put entire timestamp in columns[3]

  QString timestamp;
  timestamp.sprintf("%s-%s-%s %s", year.utf8().data(),
		    columns[8], columns[3], columns[10]);

  kdDebug(1601) << "Year is: " << year << "; Month is: " << columns[8] << "; Day is: " << columns[3] << "; Time is: " << columns[10] << endl;

  strcpy(columns[3], timestamp.ascii());

  kdDebug(1601) << "The actual file is " << filename << endl;

  QStringList list;
  list.append(QString::fromLocal8Bit(filename));
  for (int i=0; i<8; i++)
    {
      list.append(QString::fromLocal8Bit(columns[i]));
    }
  m_gui->add(&list); // send to GUI
}

void RarArch::open()
{
  kdDebug(1601) << "+RarArch::open" << endl;
  setHeaders();

  m_buffer[0] = '\0';
  m_header_removed = false;
  m_finished = false;
  
  
  KProcess *kp = new KProcess;
  *kp << m_archiver_program << "vt" << m_filename.local8Bit();
  connect( kp, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(slotReceivedTOC(KProcess*, char*, int)));
  connect( kp, SIGNAL(receivedStderr(KProcess*, char*, int)),
	   this, SLOT(slotReceivedOutput(KProcess*, char*, int)));

  connect( kp, SIGNAL(processExited(KProcess*)), this,
	   SLOT(slotOpenExited(KProcess*)));

  if (kp->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false)
    {
      KMessageBox::error( 0, i18n("Couldn't start a subprocess.") );
      emit sigOpen(this, false, QString::null, 0 );
    }

  kdDebug(1601) << "-RarArch::open" << endl;
}

void RarArch::setHeaders()
{
  kdDebug(1601) << "+RarArch::setHeaders" << endl;
  QStringList list;
  list.append(FILENAME_STRING);
  list.append(SIZE_STRING);
  list.append(PACKED_STRING);
  list.append(RATIO_STRING);
  list.append(TIMESTAMP_STRING);
  list.append(PERMISSION_STRING);
  list.append(CRC_STRING);
  list.append(METHOD_STRING);
  list.append(VERSION_STRING);

  // which columns to align right
  int *alignRightCols = new int[3];
  alignRightCols[0] = 1;
  alignRightCols[1] = 2;
  alignRightCols[2] = 3;

  m_gui->setHeaders(&list, alignRightCols, 3);
  delete [] alignRightCols;

  kdDebug(1601) << "-RarArch::setHeaders" << endl;
}


void RarArch::slotReceivedTOC(KProcess*, char* _data, int _length)
{
  kdDebug(1601) << "+RarArch::slotReceivedTOC" << endl;
  char c = _data[_length];
  _data[_length] = '\0';
	
  m_settings->appendShellOutputData( _data );

  char line[1024] = "";
  char *tmpl = line;

  char *tmpb;

  for( tmpb = m_buffer; *tmpb != '\0'; tmpl++, tmpb++ )
    *tmpl = *tmpb;

  for( tmpb = _data; *tmpb != '\n'; tmpl++, tmpb++ )
    *tmpl = *tmpb;
		
  tmpb++;
  *tmpl = '\0';

  if( *tmpb == '\0' )
    m_buffer[0]='\0';

  if( !strstr( line, "----" ) )
    {
      if( m_header_removed && !m_finished )
	{
	  processLine( line );
	}
    }
  else if(!m_header_removed)
    m_header_removed = true;
  else
    m_finished = true;

  bool stop = (*tmpb == '\0');

  while( !stop && !m_finished )
    {
      tmpl = line; *tmpl = '\0';

      for(; (*tmpb!='\n') && (*tmpb!='\0'); tmpl++, tmpb++)
	*tmpl = *tmpb;

      if( *tmpb == '\n' )
	{
	  *tmpl = '\n';
	  tmpl++;
	  *tmpl = '\0';
	  tmpb++;

	if( !strstr( line, "----" ) )
	  {
	    if( m_header_removed ){
	      processLine( line );
	    }
	  }
	else if( !m_header_removed )
	  m_header_removed = true;
	else
	  {
	    m_finished = true;
	  }
	}
      else if (*tmpb == '\0' )
	{
	  *tmpl = '\0';
	  strcpy( m_buffer, line );
	  stop = true;
	}
    }
  
  _data[_length] = c;
  kdDebug(1601) << "-RarArch::slotReceivedTOC" << endl;
}

void RarArch::create()
{
  emit sigCreate(this, true, m_filename,
		 Arch::Extract | Arch::Delete | Arch::Add 
		 | Arch::View);
}

void RarArch::addDir(const QString & _dirName)
{
  if (! _dirName.isEmpty())
  {
    QStringList list;
    list.append(_dirName);
    addFile(&list);
  }
}

void RarArch::addFile( QStringList *urls )
{
  kdDebug(1601) << "+RarArch::addFile" << endl;
  KProcess *kp = new KProcess;
  kp->clearArguments();
  *kp << m_archiver_program;

  if (m_settings->getRarReplaceOnlyWithNewer() )
    *kp << "u";
  else
    *kp << "a";

  if (m_settings->getRarStoreSymlinks())
    *kp << "-ol";
  if (m_settings->getRarRecurseSubdirs())
    *kp << "-r";

  *kp << m_filename.local8Bit() ;

  QString base;
  QString url;
  QString file;

	
  QStringList::ConstIterator iter;
  for (iter = urls->begin(); iter != urls->end(); ++iter )
  {
    url = *iter;
    // comment out for now until I figure out what happened to this function!
    //    KURL::decodeURL(url); // Because of special characters
    file = url.right(url.length()-5);

    if( file[file.length()-1]=='/' )
      file[file.length()-1]='\0';
    if( ! m_settings->getaddPath() )
    {
      int pos;
      pos = file.findRev( '/' );
      base = file.left( pos );
      pos++;
      chdir( QFile::encodeName(base) );
      base = file.right( file.length()-pos );
      file = base;
    }
    *kp << file;
  }
  connect( kp, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(slotReceivedOutput(KProcess*, char*, int)));
  connect( kp, SIGNAL(receivedStderr(KProcess*, char*, int)),
	   this, SLOT(slotReceivedOutput(KProcess*, char*, int)));

  connect( kp, SIGNAL(processExited(KProcess*)), this,
	   SLOT(slotAddExited(KProcess*)));

  if (kp->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false)
    {
      KMessageBox::error( 0, i18n("Couldn't start a subprocess.") );
      emit sigAdd(false);
    }

  kdDebug(1601) << "+RarArch::addFile" << endl;
}

void RarArch::unarchFile(QStringList *_fileList, const QString & _destDir)
{
  // if _fileList is empty, we extract all.
  // if _destDir is empty, look at settings for extract directory

  kdDebug(1601) << "+RarArch::unarchFile" << endl;

  QString dest;
  if (_destDir.isEmpty() || _destDir.isNull())
    dest = m_settings->getExtractDir();
  else dest = _destDir;

  KProcess *kp = new KProcess;
  kp->clearArguments();

  // extract (and maybe overwrite)
  *kp << m_unarchiver_program << "x";

  if (!m_settings->getRarOverwriteFiles())
    {
      *kp << "-o+" ; 
    }
  else
    {
    *kp << "-o-" ;
    }

#if 0
  if (g_pSettings->filesToLower())
  {
    *kp << "-cl";
  }
#endif

  *kp << m_filename.local8Bit();

  // if the file list is empty, no filenames go on the command line,
  // and we then extract everything in the archive.
  if (_fileList)
    {
      for ( QStringList::Iterator it = _fileList->begin();
	    it != _fileList->end(); ++it ) 
	{
	  *kp << (*it).latin1() ;
	}
    }

  *kp << dest ;
 
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
}

void RarArch::remove(QStringList *list)
{
  kdDebug(1601) << "+RarArch::remove" << endl;

  if (!list)
    return;

  m_shellErrorData = "";
  KProcess *kp = new KProcess;
  kp->clearArguments();
  
  *kp << m_archiver_program << "d" << m_filename.local8Bit();
  for ( QStringList::Iterator it = list->begin();
	it != list->end(); ++it )
    {
      QString str = *it;
      *kp << str.local8Bit();
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
  
  kdDebug(1601) << "-RarArch::remove" << endl;
}



#include "rar.moc"
