/*

 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org
 1999-2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)
 2001: Corel Corporation (author: Michael Jarrett, michaelj@corel.com)

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

// KDE includes
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprocess.h>

// ark includes
#include "arkwidgetbase.h"
#include "arksettings.h"
#include "zip.h"

ZipArch::ZipArch( ArkSettings *_settings, ArkWidgetBase *_gui,
		  const QString & _fileName )
  : Arch(_settings, _gui, _fileName )
{
  m_archiver_program = "zip";
  m_unarchiver_program = "unzip";
  verifyUtilityIsAvailable(m_archiver_program, m_unarchiver_program);

  m_headerString = "----";
  m_repairYear = 9; m_fixMonth = 7; m_fixDay = 8; m_fixTime = 10;
  m_dateCol = 5;
  m_numCols = 7;

  m_archCols.append(new ArchColumns(1, QRegExp("[0-9]+")));
  m_archCols.append(new ArchColumns(2, QRegExp("[^\\s]+")));
  m_archCols.append(new ArchColumns(3, QRegExp("[0-9]+")));
  m_archCols.append(new ArchColumns(4, QRegExp("[0-9.]+%")));
  m_archCols.append(new ArchColumns(7, QRegExp("[01][0-9]"), 2));
  m_archCols.append(new ArchColumns(8, QRegExp("[0-3][0-9]"), 2));
  m_archCols.append(new ArchColumns(9, QRegExp("[0-9][0-9]"), 2));
  m_archCols.append(new ArchColumns(10, QRegExp("[0-9:]+"), 6));
  m_archCols.append(new ArchColumns(6, QRegExp("[a-fA-F0-9]+")));
  m_archCols.append(new ArchColumns(0, QRegExp("[^\\s][^\\n]+"), 4096));

  kdDebug(1601) << "ZipArch constructor" << endl;
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

  m_buffer = "";
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
    bool bOldRecVal = m_settings->getZipAddRecurseDirs();
    
    // must be true for add directory - otherwise why would user try?
    m_settings->setZipAddRecurseDirs(true);

    QStringList list;
    list.append(_dirName);
    addFile(&list);
    m_settings->setZipAddRecurseDirs(bOldRecVal); // reset to old val
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

  if (m_settings->getZipStoreSymlinks())
    *kp << "-y";

//  if (m_settings->getZipAddJunkDirs())	// Extraneous
//    *kp << "-j";

  if (m_settings->getZipAddMSDOS())
    *kp << "-k";
  if (m_settings->getZipAddConvertLF())
    *kp << "-l";
  
  if (m_settings->getAddReplaceOnlyWithNewer())
    *kp << "-u";

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
      chdir(QFile::encodeName(base));
      base = file.right(file.length()-pos);
      file = base;
    }
    *kp << file;
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
	   SLOT(slotAddExited(KProcess*)));

  if (kp->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false)
    {
      KMessageBox::error( 0, i18n("Couldn't start a subprocess.") );
      emit sigAdd(false);
    }

  kdDebug(1601) << "-ZipArch::addFile" << endl;
}

void ZipArch::unarchFile(QStringList *_fileList, const QString & _destDir,
			 bool viewFriendly)
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
  
  *kp << m_unarchiver_program;

  if (m_settings->getZipExtractJunkPaths() && !viewFriendly)
    *kp << "-j" ;

  if (m_settings->getZipExtractLowerCase())
    *kp << "-L";

  if (m_settings->getExtractOverwrite())
    *kp << "-o";

  *kp << m_filename;
  
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
	  KMessageBox::error( 0, i18n("You probably don't have sufficient permissions.\n"
				      "Please check the file owner and the integrity "
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
