/*

 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

 2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)
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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <config.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <string.h>

// QT includes
#include <qfile.h>
#include <qdir.h>

// KDE includes
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprocess.h>

// ark includes
#include "settings.h"
#include "arkwidget.h"
#include "arch.h"
#include "zoo.h"
#include "arkutils.h"

QString fixTime(const QString &_strTime);

ZooArch::ZooArch( ArkWidget *_gui, const QString & _fileName )
  : Arch( _gui, _fileName )
{
  kdDebug(1601) << "ZooArch constructor" << endl;
  m_archiver_program = "zoo";
  m_unarchiver_program = QString::null;
  verifyUtilityIsAvailable(m_archiver_program, m_unarchiver_program);

  m_headerString = "----";
}

bool ZooArch::processLine(const QCString &line)
{
  const char *_line = (const char *)line;
  char columns[11][80];
  char filename[4096];

  // Note: I'm reversing the ratio and the length for better display

  sscanf(_line, " %79[0-9] %79[0-9%] %79[0-9] %79[0-9] %79[a-zA-Z] %79[0-9]%79[ ]%11[ 0-9:+-]%2[C ]%4095[^\n]",
	 columns[1], columns[0], columns[2], columns[3], columns[7],
	 columns[8], columns[9], columns[4], columns[10], filename);


  kdDebug(1601) << "The actual file is " << (const char *)filename << endl;

  QString year = ArkUtils::fixYear(columns[8]);

  kdDebug(1601) << "The year is " << year << endl;

  QString strDate;
  strDate.sprintf("%s-%.2d-%.2d", year.utf8().data(),
		    ArkUtils::getMonth(columns[7]), atoi(columns[3]));

  strlcpy(columns[3], strDate.ascii(), sizeof(columns[3]));
  kdDebug(1601) << "New timestamp is " << columns[3] << endl;

  strlcat(columns[3], " ", sizeof(columns[3]));
  strlcat(columns[3], fixTime(columns[4]).ascii(),sizeof(columns[3]));

  QStringList list;
  list.append(QFile::decodeName(filename));
  for (int i=0; i<4; i++)
    {
      list.append(QString::fromLocal8Bit(columns[i]));
    }
  m_gui->listingAdd(&list); // send to GUI

  return true;
}

void ZooArch::open()
{
  kdDebug(1601) << "+ZooArch::open" << endl;
  setHeaders();

  m_buffer = "";
  m_header_removed = false;
  m_finished = false;


  KProcess *kp = new KProcess;
  *kp << m_archiver_program << "l" << m_filename;
  connect( kp, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(slotReceivedTOC(KProcess*, char*, int)));
  connect( kp, SIGNAL(receivedStderr(KProcess*, char*, int)),
	   this, SLOT(slotReceivedOutput(KProcess*, char*, int)));

  connect( kp, SIGNAL(processExited(KProcess*)), this,
	   SLOT(slotOpenExited(KProcess*)));

  if (kp->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false)
    {
      KMessageBox::error( 0, i18n("Could not start a subprocess.") );
      emit sigOpen(this, false, QString::null, 0 );
    }

  kdDebug(1601) << "-ZooArch::open" << endl;
}

void ZooArch::setHeaders()
{
  kdDebug(1601) << "+ZooArch::setHeaders" << endl;
  QStringList list;
  list.append(FILENAME_STRING);
  list.append(RATIO_STRING);
  list.append(SIZE_STRING);
  list.append(PACKED_STRING);
  list.append(TIMESTAMP_STRING);

  // which columns to align right
  int *alignRightCols = new int[3];
  alignRightCols[0] = 2;
  alignRightCols[1] = 3;
  alignRightCols[2] = 4;

  m_gui->setHeaders(&list, alignRightCols, 3);
  delete [] alignRightCols;

  kdDebug(1601) << "-ZooArch::setHeaders" << endl;
}


void ZooArch::create()
{
  emit sigCreate(this, true, m_filename,
		 Arch::Extract | Arch::Delete | Arch::Add
		 | Arch::View);
}

void ZooArch::addDir(const QString & _dirName)
{
  if (! _dirName.isEmpty())
  {
    QStringList list;
    list.append(_dirName);
    addFile(&list);
  }
}

void ZooArch::addFile( QStringList *urls )
{
  kdDebug(1601) << "+ZooArch::addFile" << endl;
  KProcess *kp = new KProcess;
  kp->clearArguments();
  *kp << m_archiver_program;

  if (Settings::replaceOnlyWithNewer())
    *kp << "-update";
  else
    *kp << "-add";

  *kp << m_filename;

  QStringList::ConstIterator iter;
  for (iter = urls->begin(); iter != urls->end(); ++iter )
  {
    KURL url( *iter );

    if( !Settings::addDir() )
    {
      QDir::setCurrent(url.directory());
      *kp << url.fileName();
    }
    else
      *kp << url.path(-1);
  }
  connect( kp, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(slotReceivedOutput(KProcess*, char*, int)));
  connect( kp, SIGNAL(receivedStderr(KProcess*, char*, int)),
	   this, SLOT(slotReceivedOutput(KProcess*, char*, int)));

  connect( kp, SIGNAL(processExited(KProcess*)), this,
	   SLOT(slotAddExited(KProcess*)));

  if (kp->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false)
    {
      KMessageBox::error( 0, i18n("Could not start a subprocess.") );
      emit sigAdd(false);
    }

  kdDebug(1601) << "+ZooArch::addFile" << endl;
}

void ZooArch::unarchFile(QStringList *_fileList, const QString & _destDir,
			 bool viewFriendly)
{
  // if _fileList is empty, we extract all.
  // if _destDir is empty, abort with error.

  kdDebug(1601) << "+ZooArch::unarchFile" << endl;
  QString dest;

  if (_destDir.isEmpty() || _destDir.isNull())
    {
      kdError(1601) << "There was no extract directory given." << endl;
      return;
    }
  else dest = _destDir;

  // zoo has no option to specify the destination directory
  // so I have to change to it.

  bool ret = QDir::setCurrent(dest);
 // I already checked the validity of the dir before coming here
  Q_ASSERT(ret);


  QString tmp;

  KProcess *kp = new KProcess;
  kp->clearArguments();

  *kp << m_archiver_program;

  if (!Settings::extractOverwrite())
    *kp << "x";
  else
    *kp << "xOOS";
  *kp << m_filename;

  // if the list is empty, no filenames go on the command line,
  // and we then extract everything in the archive.
  if (_fileList)
    {
      for ( QStringList::Iterator it = _fileList->begin();
	    it != _fileList->end(); ++it )
	{
	  *kp << (*it);/*.latin1() ;*/
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
      KMessageBox::error( 0, i18n("Could not start a subprocess.") );
      emit sigExtract(false);
    }
}

void ZooArch::remove(QStringList *list)
{
  kdDebug(1601) << "+ZooArch::remove" << endl;

  if (!list)
    return;

  m_shellErrorData = "";
  KProcess *kp = new KProcess;
  kp->clearArguments();

  *kp << m_archiver_program << "D" << m_filename;
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
      KMessageBox::error( 0, i18n("Could not start a subprocess.") );
      emit sigDelete(false);
    }

  kdDebug(1601) << "-ZooArch::remove" << endl;
}

QString fixTime(const QString &_strTime)
{
  // it may have come from a different time zone... get rid of trailing
  // +3 or -3 etc.
  QString strTime = _strTime;

  if (strTime.contains("+") || strTime.contains("-"))
    {
      QCharRef c = strTime.at(8);
      int offset = strTime.right(strTime.length() - 9).toInt();
      kdDebug(1601) << "Offset is " << offset << "\n" << endl;
      QString strHour = strTime.left(2);
      int nHour = strHour.toInt();
      if (c == '+' || c == '-')
	{
	  if (c == '+')
	    nHour = (nHour + offset) % 24;
	  else if (c == '-')
	    {
	      nHour -= offset;
	      if (nHour < 0)
		nHour += 24;
	    }
	  strTime = strTime.left(8);
	  strTime.sprintf("%2.2d%s", nHour, strTime.right(6).utf8().data());
	  kdDebug(1601) << "The new time is " << strTime << endl;
	}
    }
  else
    {
      strTime = strTime.left(8);
    }
  return strTime;
}

#include "zoo.moc"
