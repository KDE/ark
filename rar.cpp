/*

 ark -- archiver for the KDE project

 Copyright (C)

 2003: Helio Chissini de Castro <helio@conectiva.com>
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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// Std includes
#include <sys/errno.h>
#include <unistd.h>
#include <iostream>
#include <string>

// QT includes
#include <qfile.h>
#include <qdir.h>

// KDE includes
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <kstandarddirs.h>

// ark includes
#include <config.h>
#include "arkwidgetbase.h"
#include "arch.h"
#include "arksettings.h"
#include "rar.h"
#include "arkutils.h"

RarArch::RarArch( ArkSettings *_settings, ArkWidgetBase *_gui,
		  const QString & _fileName )
  : Arch(_settings, _gui, _fileName ), m_linenumber(0)
{
  kdDebug(1601) << "RarArch constructor" << endl;

  bool have_rar = !KGlobal::dirs()->findExe( "rar" ).isNull();
  bool have_unrar = !KGlobal::dirs()->findExe( "unrar" ).isNull();

  m_archiver_program = have_rar ? "rar" : "unrar";
  m_unarchiver_program = have_unrar ? "unrar" : "rar";
      
  verifyUtilityIsAvailable(m_archiver_program, m_unarchiver_program);

  m_headerString = "----";
}

bool RarArch::processLine(const QCString &line)
{
  // For each rar entry, this function is called exactly three times.
  // The first time, store the first line and return.
  // The second time, store the second line and return.
  // The third time, process the data in the first two lines and
  // send to the GUI. We ignore the third line since the data there
  // isn't really that important.
  const char *_line = (const char *)line;

  ++m_linenumber;
  if (m_linenumber == 1)
    {
      m_line1 = QString::fromLocal8Bit(_line);
      return true;
    }
  if (m_linenumber == 2)
    {
      m_line2 = QString::fromLocal8Bit(_line);
      return true;
    }
  // if we made it here, we have all three lines.
  // Reset the line number.
  m_linenumber = 0;

  char columns[11][80];
  char filename[4096];
  sscanf(QFile::encodeName(m_line1), " %4095[^\n]", filename);
  sscanf((const char *)m_line2.ascii(), " %79[0-9] %79[0-9] %79[0-9%<>-] %2[0-9]-%2[0-9]-%2[0-9] %5[0-9:] %79[drwxlst-] %79[A-F0-9] %79[A-Za-z0-9] %79[0-9.]",
	 columns[0], columns[1], columns[2], columns[3],
	 columns[8], columns[9], columns[10],
	 columns[4], columns[5], columns[6],
	 columns[7]);

  // rearrange columns 3, 8, 9 so that the sort will work.
  // columns[3] is the day
  // columns[8] is the month
  // columns[9] is a 2-digit year. Ugh. Y2K junk here.
  
  QString year = ArkUtils::fixYear(columns[9]);

  // put entire timestamp in columns[3]

  QString timestamp;
  timestamp.sprintf("%s-%s-%s %s", year.utf8().data(),
		    columns[8], columns[3], columns[10]);

  //kdDebug(1601) << "Year is: " << year << "; Month is: " << columns[8] << "; Day is: " << columns[3] << "; Time is: " << columns[10] << endl;

  strlcpy(columns[3], timestamp.ascii(), sizeof(columns[3]));

  //kdDebug(1601) << "The actual file is " << filename << endl;

  QStringList list;
  list.append(QFile::decodeName(filename));
  for (int i=0; i<8; i++)
    {
      list.append(QString::fromLocal8Bit(columns[i]));
    }
  m_gui->listingAdd(&list); // send to GUI

  return true;
}

void RarArch::open()
{
  kdDebug(1601) << "+RarArch::open" << endl;
  setHeaders();

  m_buffer = "";
  m_header_removed = false;
  m_finished = false;
  
  
  KProcess *kp = new KProcess;
  *kp << m_archiver_program << "vt" << m_filename;
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

  if (m_settings->getAddReplaceOnlyWithNewer() )
    *kp << "u";
  else
    *kp << "a";

  if (m_settings->getRarStoreSymlinks())
    *kp << "-ol";
  if (m_settings->getRarRecurseSubdirs())
    *kp << "-r";

  *kp << m_filename;

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
      QDir::setCurrent(base);
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

  kdDebug(1601) << "-RarArch::addFile" << endl;
}

void RarArch::unarchFile(QStringList *_fileList, const QString & _destDir,
			 bool viewFriendly)
{
  kdDebug(1601) << "+RarArch::unarchFile" << endl;

  QString dest;
  if (_destDir.isEmpty() || _destDir.isNull())
    {
      kdError(1601) << "There was no extract directory given." << endl;
      return;
    }
  else dest = _destDir;

  KProcess *kp = new KProcess;
  kp->clearArguments();

  // extract (and maybe overwrite)
  *kp << m_unarchiver_program << "x";

  if (!m_settings->getExtractOverwrite())
    {
      *kp << "-o+" ; 
    }
  else
    {
    *kp << "-o-" ;
    }

  *kp << m_filename;

  // if the file list is empty, no filenames go on the command line,
  // and we then extract everything in the archive.
  if (_fileList)
    {
      for ( QStringList::Iterator it = _fileList->begin();
	    it != _fileList->end(); ++it ) 
	{
	  *kp << (*it);/*.latin1() ;*/
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
  
  *kp << m_archiver_program << "d" << m_filename;
  for ( QStringList::Iterator it = list->begin();
	it != list->end(); ++it )
    {
      QString str = *it;
      *kp << str;
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
