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

#include <stdlib.h>
#include <time.h>

#include <qapplication.h>
#include <qfile.h>

#include <kdebug.h>
#include <kmessagebox.h>
#include <kmimemagic.h>
#include <klocale.h>

#include <stdlib.h>

// ark includes
#include "arch.h"
#include "viewer.h"

Arch::Arch( ArkSettings *_settings, Viewer *_viewer,
	    const QString & _fileName )
  : m_filename(_fileName), m_settings(_settings), m_gui(_viewer),
    m_bReadOnly(false), m_bNotifyWhenDeleteFails(true)
{
  kdDebug(1601) << "+Arch::Arch" << endl;
  kdDebug(1601) << "-Arch::Arch" << endl;
}

Arch::~Arch()
{
}

void Arch::verifyUtilityIsAvailable(const QString & _utility1,
			      const QString & _utility2)
{
  // see if the utility is in the PATH of the user. If there is a
  // second utility specified, it must also be present.
  QString cmd = "which " + _utility1;
  int ret1 = system(QFile::encodeName(cmd));
  int ret2 = 0;
  if (!_utility2.isNull())
    {
      cmd = "which " + _utility2;
      ret2 = system(QFile::encodeName(cmd));
    }

  m_bUtilityIsAvailable = ((ret1 == 0) && (ret2 == 0));
}

void Arch::slotCancel()
{
  //  m_kp->kill();
}

void Arch::slotStoreDataStdout(KProcess*, char* _data, int _length)
{
  char c = _data[_length];
  _data[_length] = '\0';

  m_settings->appendShellOutputData( _data );
  _data[_length] = c;
}

void Arch::slotStoreDataStderr(KProcess*, char* _data, int _length)
{
  char c = _data[_length];
  _data[_length] = '\0';
	
  m_shellErrorData.append( _data );
  _data[_length] = c;
}

void Arch::slotOpenExited(KProcess* _kp)
{
  kdDebug(1601) << "normalExit = " << _kp->normalExit() << endl;
  kdDebug(1601) << "exitStatus = " << _kp->exitStatus() << endl;

  bool bNormalExit = _kp->normalExit();

  int exitStatus = 100; // arbitrary bad exit status
  if (bNormalExit)
    exitStatus = _kp->exitStatus();

  if (1 == exitStatus)
    exitStatus = 0;    // because 1 means empty archive - not an error.
                       // Is this a safe assumption?

  if(!exitStatus) 
    emit sigOpen( this, true, m_filename,
		  Arch::Extract | Arch::Delete | Arch::Add | Arch::View );
  else
    emit sigOpen( this, false, QString::null, 0 );

  delete _kp;
  _kp = NULL;

}

void Arch::slotDeleteExited(KProcess *_kp)
{
  kdDebug(1601) << "+Arch::slotDeleteExited" << endl;

  bool bSuccess = false;

  kdDebug(1601) << "normalExit = " << _kp->normalExit() << endl;
  if( _kp->normalExit() )
    kdDebug(1601) << "exitStatus = " << _kp->exitStatus() << endl;
  
  if( _kp->normalExit() && (_kp->exitStatus()==0) )
    {
      if(stderrIsError())
	{
	  QApplication::restoreOverrideCursor();
	  KMessageBox::error( 0, i18n("You probably don't have sufficient permissions.\n"
				      "Please check the file owner and the integrity\n"
				      "of the archive.") );
	}
      else
	bSuccess = true;
    }
  else
    {
      if (m_bNotifyWhenDeleteFails)
	{
	  QApplication::restoreOverrideCursor();
	  KMessageBox::sorry( (QWidget *)0,
			      i18n("Deletion failed"), i18n("Error") );
	}
      else bSuccess = true;
    }
  
  emit sigDelete(bSuccess);
  delete _kp;
  _kp = NULL;

  kdDebug(1601) << "-Arch::slotDeleteExited" << endl;
}

void Arch::slotExtractExited(KProcess *_kp)
{
  kdDebug(1601) << "+Arch::slotExtractExited" << endl;

  bool bSuccess = false;

  kdDebug(1601) << "normalExit = " << _kp->normalExit() << endl;
  if( _kp->normalExit() )
    kdDebug(1601) << "exitStatus = " << _kp->exitStatus() << endl;

  if( _kp->normalExit() && (_kp->exitStatus()==0) )
    {
      if(stderrIsError())
	{
	  QApplication::restoreOverrideCursor();
	  int ret = KMessageBox::warningYesNo( (QWidget *) 0, i18n("Sorry, the extract operation failed.\nDo you wish to view the shell output?"), i18n("Error"));
	  if (ret == KMessageBox::Yes)
	    m_gui->viewShellOutput();
	}
      else
	bSuccess = true;
    }

  emit sigExtract(bSuccess);
  delete _kp;
  _kp = NULL;

  kdDebug(1601) << "-Arch::slotExtractExited" << endl;
}

void Arch::slotAddExited(KProcess *_kp)
{
  kdDebug(1601) << "+Arch::slotAddExited" << endl;

  bool bSuccess = false;

  kdDebug(1601) << "normalExit = " << _kp->normalExit() << endl;
  if( _kp->normalExit() )
    kdDebug(1601) << "exitStatus = " << _kp->exitStatus() << endl;

  if( _kp->normalExit() && (_kp->exitStatus()==0) )
    {
      if(stderrIsError())
	{
	  QApplication::restoreOverrideCursor();
	  KMessageBox::error( 0, i18n("You probably don't have sufficient permissions\n"
				      "Please check the file owner and the integrity\n"
				      "of the archive.") );
	}
      else
	bSuccess = true;
    }
  else
    {
      QApplication::restoreOverrideCursor();
      int ret = KMessageBox::warningYesNo( (QWidget *) 0, i18n("Sorry, the add operation failed.\nDo you wish to view the shell output?"), i18n("Error"));
	  if (ret == KMessageBox::Yes)
	    m_gui->viewShellOutput();
    }  
  emit sigAdd(bSuccess);
  delete _kp;
  _kp = NULL;

  kdDebug(1601) << "-Arch::slotAddExited" << endl;
}


bool Arch::stderrIsError()
{
  return m_shellErrorData.find(QString("eror")) != -1;
}

void Arch::slotReceivedOutput(KProcess*, char* _data, int _length)
{
  char c = _data[_length];
  _data[_length] = '\0';

  m_settings->appendShellOutputData( _data );
  _data[_length] = c;
}

/// UTILS


QString Utils::getTimeStamp(const QString &_month,
			    const QString &_day,
			    const QString &_yearOrTime)
{
  // Make the date format sortable.
  // Month is in _month, day is in _day.
  // In _yearOrTime is either a year or a time. 
  // If it's March, we'll see the year for all dates up to October 1999.
  // (five months' difference - e.g., if it's Apr, then get years up to Nov)

  char month[4];
  strncpy(month, _month.ascii(), 3);
  month[3] = '\0';
  int nMonth = getMonth(month);
  int nDay = _day.toInt();

  kdDebug(1601) << "Month is " << nMonth << ", Day is " << nDay << endl;

  time_t t = time(0);
  if (t == -1)
    exit(1);
  struct tm *now = localtime(&t);
  int thisYear = now->tm_year + 1900;
  int thisMonth = now->tm_mon + 1;

  QString year, timestamp;

  if (_yearOrTime.contains(":"))
    // it has a timestamp so we have to figure out the year
    {
      year.sprintf("%d", Utils::getYear(nMonth, thisYear, thisMonth));
      timestamp = _yearOrTime;
    }
  else
    {
      year = _yearOrTime;
      if (year.right(1) == " ")
	year = year.left(4);
      if (year.left(1) == " ")
	year = year.right(4);

      timestamp = "??:??";
    }
    
  QString retval;
  retval.sprintf("%s-%.2d-%.2d %s",
		 year.utf8().data(), nMonth, nDay, 
		 timestamp.utf8().data());
  return retval;
}

int Utils::getMonth(const char *strMonth)
  // returns numeric value for three-char month string
{
  static char months[13][4] = { "", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
				"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  int nIndex;
  for (nIndex = 1; nIndex < 13; ++nIndex)
    {
      if (0 == strcmp(strMonth, months[nIndex]))
	return nIndex;
    }
  return 0;
}
  
// This function gets the year from an LHA or ls -l timestamp.
// Note: LHA doesn't seem to display the year if the file is more
// than 6 months into the future, so this will fail to give the correct
// year (of course it is hoped that there are not too many files lying 
// around from the future).

int Utils::getYear(int theMonth, int thisYear, int thisMonth)
{
  int monthDiff = QABS(thisMonth - theMonth);
  if (monthDiff > 6)
    return (thisYear - 1);
  else
    return thisYear;
}

QString Utils::fixYear(const char *strYear)
{
  // returns 4-digit year by guessing from two-char year string.
  // Remember: this is used for file timestamps. There probably aren't any
  // files that were created before 1970, so that's our cutoff. Of course,
  // in 2070 we'll have some problems....

  char fourDigits[5] = {0,0,0,0,0};
  if (atoi(strYear) > 70)
    {
      strcpy(fourDigits, "19");
    }
  else
    {
      strcpy(fourDigits, "20");
    }
  strcat(fourDigits, strYear);
  return fourDigits;
}

ArchType getArchType( const QString & archname, QString &extension)
{
  if ((archname.right(4) == ".tgz") || (archname.right(4) == ".tzo")
       || (archname.right(4) == ".taz")
       || (archname.right(4) == ".tar"))
    {
      extension = archname.right(4);
      return TAR_FORMAT;
    }
  if (archname.right(6) == ".tar.Z")
    {
      extension = ".tar.Z";
      return TAR_FORMAT;
    }
  if ((archname.right(7) == ".tar.gz")
      || (archname.right(7) == ".tar.bz"))
    {
      extension = archname.right(7);
      return TAR_FORMAT;
    }
  if ((archname.right(8) == ".tar.bz2")
      || (archname.right(8) == ".tar.lzo"))
    {
      extension = archname.right(8);
      return TAR_FORMAT;
    }
  if ((archname.right(4) == ".lha") || (archname.right(4) == ".lzh"))
    {
      extension = archname.right(4);
      return LHA_FORMAT;
    }
  if ((archname.right(4) == ".zip") || (archname.right(4) == ".xpi"))
    {
      extension = archname.right(4);
      return ZIP_FORMAT;
    }
  if ((archname.right(3) == ".gz") || (archname.right(3) == ".bz"))
    {
      extension = archname.right(3);
      return COMPRESSED_FORMAT;
    }
  if ((archname.right(4) == ".bz2") || (archname.right(4) == ".lzo"))
    {
      extension = archname.right(4);
      return COMPRESSED_FORMAT;
    }
  if (archname.right(2) == ".Z")
    {
      extension = ".Z";
      return COMPRESSED_FORMAT;
    }
  if (archname.right(4) == ".zoo")
    {
      extension = ".zoo";
      return ZOO_FORMAT;
    }
  if (archname.right(4) == ".rar")
    {
      extension = ".rar";
      return RAR_FORMAT;
    }
  if (archname.right(2) == ".a")
    {
      extension = ".a";
      return AA_FORMAT;
    }

  // now try using magic

  KMimeMagic *mimePtr = KMimeMagic::self();
  KMimeMagicResult * mimeResultPtr = mimePtr->findFileType(archname);
  QString mimetype = mimeResultPtr->mimeType();
  extension = QString::null;

  kdDebug(1601) << "The mime type is " << mimetype << endl;

  if (mimetype == "application/x-rar")
    return RAR_FORMAT;
  if (mimetype == "application/x-lha")
    return LHA_FORMAT;
  if (mimetype == "application/x-archive")
    return AA_FORMAT;
  if (mimetype == "application/x-tar")
    return TAR_FORMAT;
  if (mimetype == "application/x-zip")
    return ZIP_FORMAT;
  return UNKNOWN_FORMAT;
}



#include "arch.moc"



