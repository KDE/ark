/*

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
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
#include <errno.h>

// KDE includes
#include <kurl.h>
#include <qstringlist.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <klocale.h>

// ark includes
#include "ar.h"
#include "viewer.h"

ArArch::ArArch( ArkSettings *_settings, Viewer *_gui,
		  const QString & _fileName )
  : Arch(_settings, _gui, _fileName )
{
  m_archiver_program = "ar";
  kDebugInfo(1601, "ArArch constructor");
}

void ArArch::setHeaders()
{
  kDebugInfo( 1601, "+ArArch::setHeaders");
  QStringList list;
  list.append(i18n(" Filename "));
  list.append(i18n(" Permissions "));
  list.append(i18n(" Owner/Group "));
  list.append(i18n(" Size "));
  list.append(i18n(" Timestamp "));


  // which columns to align right
  int *alignRightCols = new int[1];
  alignRightCols[0] = 3;

  m_gui->setHeaders(&list, alignRightCols, 1);
  delete [] alignRightCols;

  kDebugInfo( 1601, "-ArArch::setHeaders");
}

void ArArch::processLine( char *_line )
{
  char columns[9][80];
  char filename[4096];
  if (_line[0] == '\020') // hack hack
    sscanf(_line, "\020%[-dwrxl] %[0-9/] %[0-9] %3[A-Za-z] %2[0-9 ] %5[0-9:] %4[0-9]%1[ ]%[^\n]",
	   columns[0], columns[1], columns[2], columns[3], columns[4],
	   columns[5], columns[6], columns[7], filename );
  else
    sscanf(_line, "%[-dwrxl] %[0-9/] %[0-9] %3[A-Za-z] %2[0-9 ] %5[0-9:] %4[0-9]%1[ ]%[^\n]",
	   columns[0], columns[1], columns[2], columns[3], columns[4],
	   columns[5], columns[6], columns[7], filename );
    
  
  kDebugInfo(1601, "%s!%s!%s!%s!%s!%s!%s!%s!%s",
	 columns[0], columns[1], columns[2], columns[3], columns[4],
	 columns[5], columns[6], columns[7], filename );


  // Put columns[3] - [6] into standard format
  QString timestamp;
  timestamp.sprintf("%s-%.2d-%.2d %s",
		    columns[6], Utils::getMonth(columns[3]),
		    atoi(columns[4]), columns[5]);
  // put timestamp into column 3
  strcpy(columns[3], (const char *)timestamp);
  kDebugInfo(1601, "Timestamp for file %s is %s", (const char *)filename,
	     (const char *)timestamp);

  QStringList list;
  list.append(QString::fromLocal8Bit(filename));
  for (int i=0; i<4; i++)
    {
      list.append(QString::fromLocal8Bit(columns[i]));
    }
  m_gui->add(&list); // send to GUI
}

void ArArch::open()
{
  kDebugInfo(1601, "+ArArch::open");
  setHeaders();
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
  kDebugInfo(1601, "-ArArch::open");
}

void ArArch::slotReceivedTOC(KProcess*, char* _data, int _length)
{
  kDebugInfo(1601, "+ArArch::slotReceivedTOC");
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

  processLine( line );
  bool stop = (*tmpb == '\0');

  while( !stop)
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

	  processLine( line );
	}
      else if (*tmpb == '\0' )
	{
	  *tmpl = '\0';
	  strcpy( m_buffer, line );
	  stop = true;
	}
    }
  
  _data[_length] = c;



  kDebugInfo(1601, "-ArArch::slotReceivedTOC");
}

void ArArch::create()
{
  KProcess *kp = new KProcess;
  kp->clearArguments();
  *kp << m_archiver_program << "c" << m_filename.local8Bit();

  connect( kp, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(slotReceivedOutput(KProcess*, char*, int)));
  connect( kp, SIGNAL(receivedStderr(KProcess*, char*, int)),
	   this, SLOT(slotReceivedOutput(KProcess*, char*, int)));

  if (kp->start(KProcess::Block) == false)
    {
      KMessageBox::error( 0, i18n("Couldn't start a subprocess.") );
      emit sigCreate(this, false, m_filename,
		     Arch::Extract | Arch::Delete | Arch::Add 
		     | Arch::View);
    }
  else
    emit sigCreate(this, true, m_filename,
		   Arch::Extract | Arch::Delete | Arch::Add 
		   | Arch::View);
}

void ArArch::addFile( QStringList *urls )
{
  kDebugInfo( 1601, "+ArArch::addFile");
  KProcess *kp = new KProcess;
  kp->clearArguments();
  *kp << m_archiver_program << "r";
	
  if (m_settings->getReplaceOnlyNew() )
    *kp << "u";

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
      chdir( base );
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

  kDebugInfo( 1601, "+ArArch::addFile");
}

void ArArch::unarchFile(QStringList *_fileList, const QString & _destDir)
{
  // if _fileList is empty, we extract all.
  // if _destDir is empty, look at settings for extract directory

  kDebugInfo( 1601, "+ArArch::unarchFile");
  QString dest;

  if (_destDir.isEmpty() || _destDir.isNull())
    dest = m_settings->getExtractDir();
  else dest = _destDir;

  // ar has no option to specify the destination directory
  // so I have to change to it.

  int ret = chdir((const char *)dest);
 // I already checked the validity of the dir before coming here
  ASSERT(ret == 0); 

  KProcess *kp = new KProcess;
  kp->clearArguments();
  
  *kp << m_archiver_program;
  *kp << "vx";
  *kp << m_filename;
  
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
}

void ArArch::remove(QStringList *list)
{
  kDebugInfo( 1601, "+ArArch::remove");

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
  
  kDebugInfo( 1601, "-ArArch::remove");
}


#include "ar.moc"
