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
#include "arkwidget.h"
#include "arch.h"
#include "settings.h"
#include "rar.h"
#include "arkutils.h"
#include "filelistview.h"

RarArch::RarArch( ArkWidget *_gui, const QString & _fileName )
  : Arch( _gui, _fileName )
{
  kdDebug(1601) << "RarArch constructor" << endl;

  bool have_rar = !KGlobal::dirs()->findExe( "rar" ).isNull(); // Check if rar is available

  if ( have_rar ) 
  {
    m_archiver_program = m_unarchiver_program = "rar"; // If it is, then use it as archiver and unarchiver
  }
  else
  {
    m_archiver_program = m_unarchiver_program = "unrar"; // If rar is not available, try to use unrar to open the archive read-only
    setReadOnly( true );
  }
  
  verifyUtilityIsAvailable( m_archiver_program );

  m_headerString = "-------------------------------------------------------------------------------";

  m_isFirstLine = true;
}

bool RarArch::processLine(const QCString &line)
{
  if ( m_isFirstLine )
  {
    m_fileName = line;
    m_fileName.remove( 0, 1 );
    m_isFirstLine = false;
    return true;
  }

  QStringList list;

  QStringList l2 = QStringList::split( ' ', line );

  list << m_fileName; // filename
  list << l2[ 0 ]; // size
  list << l2[ 1 ]; // packed
  list << l2[ 2 ]; // ratio

  QStringList date =  QStringList::split( '-', l2[ 3 ] );
  list << ArkUtils::fixYear( date[ 2 ].latin1() ) + "-" + date[ 1 ] + "-" + date [ 0 ] + " " + l2[4]; // date
  list << l2[ 5 ]; // attributes
  list << l2[ 6 ]; // crc
  list << l2[ 7 ]; // method
  list << l2[ 8 ]; // Version

  m_gui->fileList()->addItem(list); // send to GUI

  m_isFirstLine = true;
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
  *kp << m_archiver_program << "v" << "-c-" << m_filename;
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
    addFile(list);
  }
}

void RarArch::addFile( const QStringList & urls )
{
  kdDebug(1601) << "+RarArch::addFile" << endl;
  KProcess *kp = new KProcess;
  kp->clearArguments();
  *kp << m_archiver_program;

  if (Settings::replaceOnlyWithNewer() )
    *kp << "u";
  else
    *kp << "a";

  if (Settings::rarStoreSymlinks())
    *kp << "-ol";
  if (Settings::rarRecurseSubdirs())
    *kp << "-r";

  *kp << m_filename;

  QStringList::ConstIterator iter;
  KURL dir( urls.first() );
  QDir::setCurrent( dir.directory() );
  for (iter = urls.begin(); iter != urls.end(); ++iter )
  {
    KURL url( *iter );
    *kp << url.fileName();
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

  kdDebug(1601) << "-RarArch::addFile" << endl;
}

void RarArch::unarchFile(QStringList *_fileList, const QString & _destDir,
			 bool /*viewFriendly*/)
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

  if (!Settings::extractOverwrite())
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
      KMessageBox::error( 0, i18n("Could not start a subprocess.") );
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
      KMessageBox::error( 0, i18n("Could not start a subprocess.") );
      emit sigDelete(false);
    }

  kdDebug(1601) << "-RarArch::remove" << endl;
}

#include "rar.moc"
