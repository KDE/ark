/*

 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org

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
//#include "zipAddDlg.h"
//#include "zipExtractDlg.h"

#include "viewer.h"

void ZipArch::slotCancel()
{
  m_kp->kill();
}

void ZipArch::slotStoreDataStdout(KProcess* _p, char* _data, int _length)
{
  char c = _data[_length];
  _data[_length] = '\0';

  m_settings->appendShellOutputData( _data );
  _data[_length] = c;
}

void ZipArch::slotStoreDataStderr(KProcess* _p, char* _data, int _length)
{
  char c = _data[_length];
  _data[_length] = '\0';
	
  m_shellErrorData.append( _data );
  _data[_length] = c;
}

bool ZipArch::stderrIsError()
{
  return m_shellErrorData.find(QString("eror")) != -1;
}

void ZipArch::processLine( char *_line )
{
  char columns[8][80];
  char filename[4096];
	
  sscanf(_line, " %[0-9] %[a-zA-Z:] %[0-9] %[0-9%] %[-0-9] %[0-9:] "
	 "%[0-9a-z]%3[ ]%[^\n]",
	 columns[0], columns[1], columns[2], columns[3],
	 columns[4], columns[5], columns[6], columns[7],
	 filename);
  
  QStringList list;
  list.append(QString::fromLocal8Bit(filename));
  for (int i = 0; i < 7; ++i)
    {
      list.append(QString::fromLocal8Bit(columns[i]));
    }
  m_gui->add(&list); // send the entry to the GUI
}

void ZipArch::slotOpenExited(KProcess* _p)
{
  kdebug(0, 1601, "normalExit = %d", _p->normalExit() );
  kdebug(0, 1601, "exitStatus = %d", _p->exitStatus() );

  bool bNormalExit = _p->normalExit();

  int exitStatus = 100; // arbitrary bad exit status
  if (bNormalExit)
    exitStatus = _p->exitStatus();

  if (1 == exitStatus)
    exitStatus = 0;    // because 1 is for empty - just a warning. XXX

  if(!exitStatus) 
    emit sigOpen( true, m_filename,
		  Arch::Extract | Arch::Delete | Arch::Add | Arch::View );
  else
    emit sigOpen( false, QString::null, 0 );
  
  delete m_kp;
}

void ZipArch::slotOpenDataStdout(KProcess* _p, char* _data, int _length)
{
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
		
  tmpb++;	*tmpl = '\0';

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

      if( *tmpb == '\n' ){
	*tmpl = '\n';  	tmpl++;
	*tmpl = '\0';  	tmpb++;

	if( !strstr( line, "----" ) )
	  {
	    if( m_header_removed ){
	      processLine( line );
	    }
	  }
	else if( !m_header_removed )
	  m_header_removed = true;
	else{
	  m_finished = true;
	}
      }
      else if( *tmpb == '\0' ){
	*tmpl = '\0';
	strcpy( m_buffer, line );
	stop = true;
      }
    }

  _data[_length] = c;
}

void ZipArch::setHeaders()
{
  kdebug(0, 1601, "+ZipArch::setHeaders");
  QStringList list;

  list.append(i18n(" Name "));
  list.append(i18n(" Length "));
  list.append(i18n(" Method "));
  list.append(i18n(" Size "));
  list.append(i18n(" Ratio "));
  list.append(i18n(" Date "));
  list.append(i18n(" Time "));
  list.append(i18n(" CRC-32 "));

  // which columns to align right
  int *alignRightCols = new int[4];
  alignRightCols[0] = 1;
  alignRightCols[1] = 3;
  alignRightCols[2] = 4;
  alignRightCols[3] = 7;
  
  m_gui->setHeaders(&list, alignRightCols, 4);
  delete [] alignRightCols;

  kdebug(0, 1601, "-ZipArch::setHeaders");
}

void ZipArch::initOpen()
{
  kdebug(0, 1601, "+ZipArch::initOpen");
	
  m_buffer[0] = '\0';
  m_header_removed = false;
  m_finished = false;
	
  m_settings->clearShellOutput();

  m_kp = new KProcess();
  *m_kp << "unzip" << "-v" << m_filename.local8Bit();
	
  connect( m_kp, SIGNAL(receivedStdout(KProcess*, char*, int)), SLOT(slotOpenDataStdout(KProcess*, char*, int)));
  connect( m_kp, SIGNAL(processExited(KProcess*)), SLOT(slotOpenExited(KProcess*)));

  kdebug(0, 1601, "-ZipArch::initOpen");
}

void ZipArch::open()
{
  kdebug(0, 1601, "+ZipArch::open");

  setHeaders();
  initOpen();

  if(m_kp->start(KProcess::NotifyOnExit, KProcess::Stdout) == false)
    {
      KMessageBox::error( 0, i18n("Couldn't start a subprocess.") );  		
      emit sigOpen( false, QString::null, 0 );
    }
  kdebug(0, 1601, "-ZipArch::open");
}


void ZipArch::create()
{
  emit sigCreate( true, m_filename, Arch::Extract | Arch::Delete | Arch::Add 
		  | Arch::View);
}

int ZipArch::addDir(const QString & _dirName)
{
  int ret = FAILURE;
  if (! _dirName.isEmpty())
  {
    bool bOldVal = m_settings->getZipAddRecurseDirs();
    
    // must be true for add directory - otherwise why would user try?
    m_settings->setZipAddRecurseDirs(true);
    
    QStringList list;
    list.append(_dirName);
    ret = addFile(&list);
    m_settings->setZipAddRecurseDirs(bOldVal); // reset to old val
  }
  return ret;

}

int ZipArch::addFile( QStringList *urls )
{
  kdebug(0, 1601, "+ZipArch::addFile");
	
  int retCode;

  m_kp = new KProcess();
			
  *m_kp << "zip";
	
  if (m_settings->getZipAddRecurseDirs())
    *m_kp << "-r";
		
  //	*m_kp << _compression.local8Bit();   // for later
	
  if (m_settings->getZipAddJunkDirs())
    *m_kp << "-j";
  if (m_settings->getZipAddMSDOS())
    *m_kp << "-k";
  if (m_settings->getZipAddConvertLF())
    *m_kp << "-l";
	
#if 0
  switch( _mode )
    {
    case Update:
      *m_kp << "-u"; break;
    case Freshen:
      *m_kp << "-f"; break;
    case Move:
      *m_kp << "-m"; break;
    }
#endif

  *m_kp << m_filename.local8Bit() ;
  
  // iterate over QStringList and plunk onto command line

  kdebug(0, 1601, "Adding these files to the command line:");

  for (QStringList::Iterator it = urls->begin();
       it != urls->end(); ++it ) 
    {
      QString currFile = *it;
      // remove "file:"
      currFile = currFile.right( currFile.length()-5);
      *m_kp << currFile;
      kdebug(0, 1601, "%s", (const char *)currFile );
    }

  if( m_kp->start(KProcess::Block, KProcess::Stdout) == false)
    {
      KMessageBox::error( 0, i18n("Couldn't start a subprocess.") );
      return FAILURE;
    }
	
  kdebug(0, 1601, "normalExit = %d", m_kp->normalExit() );
  kdebug(0, 1601, "exitStatus = %d", m_kp->exitStatus() );
	
  if( m_kp->normalExit() && (m_kp->exitStatus()==0) )
    {
      if (stderrIsError())
	{
	  KMessageBox::error( 0, i18n("You probably don't have sufficient permissions\n"
				      "Please check the file owner and the integrity\n"
				      "of the archive.") );
	  retCode = FAILURE;
	}
      else
	retCode = SUCCESS;
    }
  else
    {
      KMessageBox::sorry( 0, i18n("Add failed") );
      retCode = FAILURE;
    }	
  delete m_kp;
  
  return retCode;
  kdebug(0, 1601, "+ZipArch::addFile");
}

QString ZipArch::unarchFile(QStringList *_fileList)
{
  // if _fileList is empty, we extract all.

  kdebug(0, 1601, "+ZipArch::unarchFile");
  QString dest = m_settings->getExtractDir();
  QString tmp;
	
  m_kp = new KProcess();
  
  *m_kp << "unzip" << "-o" << m_filename;
  
  // if the list is empty, no filenames go on the command line,
  // and we then extract everything in the archive.
  if (_fileList)
    {
      for ( QStringList::Iterator it = _fileList->begin();
	    it != _fileList->end(); ++it ) 
	{
	  *m_kp << (*it).latin1() ;
	}
    }
  *m_kp << "-d" << dest;
  
  if(m_kp->start(KProcess::Block, KProcess::Stdout) == false)
    {
      KMessageBox::error( 0, i18n("Subprocess wouldn't start!") );
    }
  
  kdebug(0, 1601, "normalExit = %d", m_kp->normalExit() );
  kdebug(0, 1601, "exitStatus = %d", m_kp->exitStatus() );
  
  if( m_kp->normalExit() && m_kp->exitStatus() ){
    KMessageBox::sorry( 0, "Unarch failed" );
  }
  
  delete m_kp;
  
  kdebug(0, 1601, "-ZipArch::unarchFile");
  
  return (dest+tmp);	
}

void ZipArch::remove(QStringList *list)
{
  kdebug(0, 1601, "+ZipArch::remove");

  if (!list)
    return;
  m_settings->clearShellOutput(); m_shellErrorData = "";
  m_kp = new KProcess();
  
  *m_kp << "zip" << "-d" << m_filename.local8Bit();
  for ( QStringList::Iterator it = list->begin();
	it != list->end(); ++it )
    {
      QString str = *it;
      *m_kp << str.local8Bit();
    }

  connect( m_kp, SIGNAL(receivedStdout(KProcess*, char*, int)), SLOT(slotStoreDataStdout(KProcess*, char*, int)));
  connect( m_kp, SIGNAL(receivedStderr(KProcess*, char*, int)), SLOT(slotStoreDataStderr(KProcess*, char*, int)));
  
  if(m_kp->start(KProcess::Block, KProcess::AllOutput) == false)
    {
      KMessageBox::error( 0, i18n("Couldn't start a subprocess.") );
      return;
    }
  
  kdebug(0, 1601, "normalExit = %d", m_kp->normalExit() );
  kdebug(0, 1601, "exitStatus = %d", m_kp->exitStatus() );
  
  if( m_kp->normalExit() && (m_kp->exitStatus()==0) )
    {
      if(stderrIsError())
	{
	  KMessageBox::error( 0, i18n("You probably don't have sufficient permissions.\n"
				      "Please check the file owner and the integrity\n"
				      "of the archive.") );
	}
    }
  else
    KMessageBox::sorry( 0, i18n("Deletion failed") );
  
  delete m_kp;
  
  kdebug(0, 1601, "-ZipArch::remove");
}

void ZipArch::slotExtractExited(KProcess *)
{
  kdebug(0, 1601, "+slotExtractExited");

  kdebug(0, 1601, "normalExit = %d", m_kp->normalExit() );
  if( m_kp->normalExit() )
    kdebug(0, 1601, "exitStatus = %d", m_kp->exitStatus() );

  m_wd->close();

		
  if( m_kp->normalExit() && (m_kp->exitStatus()==0) )
    {
      if(stderrIsError())
	{
	  KMessageBox::error( 0, i18n("You probably don't have sufficient permissions\n"
				      "Please check the file owner and the integrity\n"
				      "of the archive.") );
	}
    }
  else
    KMessageBox::sorry( 0, i18n("Extraction failed") );
		
  delete m_kp;
		
  kdebug(0, 1601, "-slotExtractExited");
}

void ZipArch::initExtract( bool _overwrite, bool _junkPaths, bool _lowerCase)
{
  m_settings->clearShellOutput();
  m_shellErrorData = "";

  m_kp = new KProcess();
		
  *m_kp << "unzip";
		
  if( _overwrite )
    *m_kp << "-o";
  else
    *m_kp << "-n";
	
  if( _junkPaths )
    *m_kp << "-j";
		
  if( _lowerCase )
    *m_kp << "-L";
		
  *m_kp << m_filename.local8Bit();

  connect( m_kp, SIGNAL(processExited(KProcess *)), SLOT(slotExtractExited(KProcess *)));
  connect( m_kp, SIGNAL(receivedStdout(KProcess*, char*, int)), SLOT(slotStoreDataStdout(KProcess*, char*, int)));
  connect( m_kp, SIGNAL(receivedStderr(KProcess*, char*, int)), SLOT(slotStoreDataStderr(KProcess*, char*, int)));
}

void ZipArch::slotIntegrityExited(KProcess *)
{
  kdebug(0, 1601, "+slotIntegrityExited");

  kdebug(0, 1601, "normalExit = %d", m_kp->normalExit() );
  kdebug(0, 1601, "exitStatus = %d", m_kp->exitStatus() );
		
  if( m_kp->normalExit() && (m_kp->exitStatus()==0) )
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
		
  delete m_kp;
		
  kdebug(0, 1601, "-slotIntegrityExited");
}

void ZipArch::testIntegrity()
{
  m_settings->clearShellOutput();
  m_shellErrorData = "";

  m_kp = new KProcess();
		
  *m_kp << "unzip -t";
		
  *m_kp << m_filename;

  connect( m_kp, SIGNAL(processExited(KProcess *)), SLOT(slotIntegrityExited(KProcess *)));
  connect( m_kp, SIGNAL(receivedStdout(KProcess*, char*, int)), SLOT(slotStoreDataStdout(KProcess*, char*, int)));
  //	connect( m_kp, SIGNAL(receivedStderr(KProcess*, char*, int)), SLOT(slotStoreDataStderr(KProcess*, char*, int)));
 		
  if(m_kp->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false)
    {
      kdebug(0, 1601, "Subprocess wouldn't start!");
      return;
    }
}

#include "zip.moc"
