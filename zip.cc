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


// Qt includes
#include <qdir.h>
#include <qstringlist.h>

// KDE includes
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

// ark includes
#include "zip.h"

// the generic viewer to which to send header and column info.
#include "viewer.h"

ZipArch::ZipArch( ArkSettings *_settings, Viewer *_gui,
		  const QString & _fileName )
  : Arch(_settings, _gui, _fileName )
{
  _settings->readZipProperties();
  m_archiver_program = "zip";
  m_unarchiver_program = "unzip";
  kdDebug(1601) << "ZipArch constructor" << endl;
}

void ZipArch::processLine( char *_line )
{
  char columns[11][80];
  char filename[4096];
	
  sscanf(_line, " %[0-9] %[a-zA-Z:] %[0-9] %[0-9%] %2[0-9]-%2[0-9]-%2[0-9]  %[0-9:] %[0-9a-z]%3[ ]%[^\n]",
	 columns[0], columns[1], columns[2], columns[3],
	 columns[4], columns[7], columns[8], columns[9],
         columns[5], columns[10], filename);

  // columns[4] is the month, columns[7] the day, columns[8] the 2-digit year,
  // columns[9] the time. Put into sortable format and plunk back into
  // columns[4]

  QString year = Utils::fixYear(columns[8]);

  kdDebug(1601) << "Year is " << (const char *)year << endl;
  QString timestamp;
  timestamp.sprintf("%s-%s-%s %s", (const char *)year,
		    columns[4], columns[7], columns[9]);
  
  strcpy(columns[4], (const char *)timestamp);
  kdDebug(1601) << "Timestamp is " << (const char *)columns[4] << endl;
  QStringList list;
  list.append(QString::fromLocal8Bit(filename));
  for (int i = 0; i < 6; ++i)
    {
      list.append(QString::fromLocal8Bit(columns[i]));
    }
  m_gui->add(&list); // send the entry to the GUI
}


void ZipArch::slotReceivedTOC(KProcess*, char* _data, int _length)
{
  kdDebug(1601) << "+ZipArch::slotReceivedTOC" << endl;
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
      if( m_header_removed && !m_finished ){
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
  kdDebug(1601) << "-ZipArch::slotReceivedTOC" << endl;
}

void ZipArch::setHeaders()
{
  kdDebug(1601) << "+ZipArch::setHeaders" << endl;
  QStringList list;

  list.append(FILENAME_STRING);
  list.append(SIZE_STRING);
  list.append(METHOD_STRING);
  list.append(PACKED_STRING);
  list.append(RATIO_STRING);
  list.append(TIMESTAMP_STRING);
  list.append(CRC_STRING);

  // which columns to align right
  int *alignRightCols = new int[6];
  alignRightCols[0] = 1;
  alignRightCols[1] = 2;
  alignRightCols[2] = 3;
  alignRightCols[3] = 4;
  alignRightCols[4] = 5;
  alignRightCols[5] = 6;
  
  m_gui->setHeaders(&list, alignRightCols, 6);
  delete [] alignRightCols;

  kdDebug(1601) << "-ZipArch::setHeaders" << endl;
}

void ZipArch::open()
{
  kdDebug(1601) << "+ZipArch::open" << endl;
  setHeaders();

  m_buffer[0] = '\0';
  m_header_removed = false;
  m_finished = false;
	
  KProcess *kp = new KProcess;

  *kp << m_unarchiver_program << "-v" << m_filename.local8Bit();
	
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

  kdDebug(1601) << "-ZipArch::open" << endl;
}


void ZipArch::create()
{
  emit sigCreate(this, true, m_filename,
		 Arch::Extract | Arch::Delete | Arch::Add 
		  | Arch::View);
}

void ZipArch::addDir(const QString & _dirName)
{
  if (! _dirName.isEmpty())
  {
    bool bOldVal = m_settings->getZipAddRecurseDirs();
    
    // must be true for add directory - otherwise why would user try?
    m_settings->setZipAddRecurseDirs(true);
    
    QStringList list;
    list.append(_dirName);
    addFile(&list);
    m_settings->setZipAddRecurseDirs(bOldVal); // reset to old val
  }
}

void ZipArch::addFile( QStringList *urls )
{
  kdDebug(1601) << "+ZipArch::addFile" << endl;
  KProcess *kp = new KProcess;
  kp->clearArguments();
			
  *kp << m_archiver_program;
	
  if (m_settings->getZipAddRecurseDirs())
    *kp << "-r";
		
  //	*kp << _compression.local8Bit();   // for later

#if 0
  if (m_settings->getZipAddJunkDirs())
    *kp << "-j";
#endif

  if (m_settings->getZipAddMSDOS())
    *kp << "-k";
  if (m_settings->getZipAddConvertLF())
    *kp << "-l";
	
#if 0
  switch( _mode )
    {
    case Update:
      *kp << "-u"; break;
    case Freshen:
      *kp << "-f"; break;
    case Move:
      *kp << "-m"; break;
    }
#endif

  *kp << m_filename.local8Bit() ;
  
  QString base;
  QString url;
  QString file;

  QStringList::ConstIterator iter;
  for (iter = urls->begin(); iter != urls->end(); ++iter )
  {
    url = *iter;
    //    KURL::decodeURL(url); // Because of special characters
    file = url.right(url.length()-5);

    if (file[file.length()-1]=='/')
      file[file.length()-1]='\0';
    if (m_settings->getZipAddJunkDirs())
    {
      int pos;
      pos = file.findRev('/');
      base = file.left(++pos);
      chdir(base);
      base = file.right(file.length()-pos);
      file = base;
    }
    *kp << file;
  }


  // iterate over QStringList and plunk onto command line

  kdDebug(1601) << "Adding these files to the command line:" << endl;

  for (QStringList::Iterator it = urls->begin();
       it != urls->end(); ++it ) 
    {
      QString currFile = *it;
      // remove "file:"
      currFile = currFile.right( currFile.length()-5);
      *kp << currFile;
      kdDebug(1601) << (const char *)currFile << endl;
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

  kdDebug(1601) << "-ZipArch::addFile" << endl;
}

void ZipArch::unarchFile(QStringList *_fileList, const QString & _destDir)
{
  // if _fileList is empty, we extract all.
  // if _destDir is empty, look at settings for extract directory

  kdDebug(1601) << "+ZipArch::unarchFile" << endl;
  QString dest;

  if (_destDir.isEmpty() || _destDir.isNull())
    dest = m_settings->getExtractDir();
  else dest = _destDir;

  QString tmp;
	
  KProcess *kp = new KProcess;
  kp->clearArguments();
  
  *kp << m_unarchiver_program << "-o" << m_filename;
  
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
  *kp << "-d" << dest;

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
  kdDebug(1601) << "-ZipArch::unarchFile" << endl;
}

void ZipArch::remove(QStringList *list)
{
  kdDebug(1601) << "+ZipArch::remove" << endl;

  if (!list)
    return;
  //  m_settings->clearShellOutput();
  m_shellErrorData = "";
  KProcess *kp = new KProcess;
  kp->clearArguments();
  
  *kp << m_archiver_program << "-d" << m_filename.local8Bit();
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
  
  kdDebug(1601) << "-ZipArch::remove" << endl;
}

void ZipArch::slotIntegrityExited(KProcess *_kp)
{
  kdDebug(1601) << "+slotIntegrityExited" << endl;

  kdDebug(1601) << "normalExit = " << _kp->normalExit() << endl;
  kdDebug(1601) << "exitStatus = " << _kp->exitStatus() << endl;
		
  if( _kp->normalExit() && (_kp->exitStatus()==0) )
    {
      if(stderrIsError())
	{
	  KMessageBox::error( 0, i18n("You probably don't have sufficient permissions\n"
				      "Please check the file owner and the integrity\n"
				      "of the archive.") );
	}
    }
  else
    KMessageBox::sorry( 0, i18n("Test of integrity failed") );

  delete _kp;
  _kp = NULL;

  kdDebug(1601) << "-slotIntegrityExited" << endl;
}

void ZipArch::testIntegrity()
{
  //  m_settings->clearShellOutput();
  m_shellErrorData = "";
  KProcess *kp = new KProcess;
  kp->clearArguments();
		
  *kp << m_unarchiver_program << "-t";
		
  *kp << m_filename;

  connect( kp, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(slotReceivedOutput(KProcess*, char*, int)));
  connect( kp, SIGNAL(receivedStderr(KProcess*, char*, int)),
	   this, SLOT(slotReceivedOutput(KProcess*, char*, int)));

  connect( kp, SIGNAL(processExited(KProcess *)), this,
	   SLOT(slotIntegrityExited(KProcess *)));
 		
  if(kp->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false)
    {
      kdDebug(1601) << "Subprocess wouldn't start!" << endl;
      return;
    }
}

#include "zip.moc"
