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

// C includes
#include <stdlib.h>
#include <time.h>

// QT includes
#include <qapplication.h>
#include <qfile.h>

// KDE includes
#include <kdebug.h>
#include <kmessagebox.h>
#include <kmimemagic.h>
#include <klocale.h>
#include <kprocess.h>

// ark includes
#include "arch.h"
#include "arksettings.h"
#include "filelistview.h"
#include "arkwidgetbase.h"

// the archive types
#include "tar.h"
#include "zip.h"
#include "lha.h"
#include "compressedfile.h"
#include "zoo.h"
#include "rar.h"
#include "ar.h"


Arch::ArchColumns::ArchColumns(int col, QRegExp reg, int length, bool opt) :
	colRef(col), pattern(reg), maxLength(length), optional(opt)
{
}


Arch::Arch(ArkSettings *_settings, ArkWidgetBase *_viewer,
	   const QString & _fileName )
  : m_filename(_fileName), m_buffer(""), m_settings(_settings),
    m_gui(_viewer), m_bReadOnly(false), m_bNotifyWhenDeleteFails(true),
    m_header_removed(false), m_finished(false),
	m_numCols(0), m_dateCol(-1), m_fixYear(-1), m_fixMonth(-1),
	m_fixDay(-1), m_fixTime(-1), m_repairYear(-1), m_repairMonth(-1),
	m_repairTime(-1)

{
	m_archCols.setAutoDelete(true);	// To check: it still leaky here???
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
	  KMessageBox::error(m_gui->getArkWidget(),
		 i18n("You probably don't have sufficient permissions.\n"
		      "Please check the file owner and the integrity "
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
	  KMessageBox::sorry(m_gui->getArkWidget(), i18n("Deletion failed"),
			     i18n("Error") );
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
	  int ret = KMessageBox::warningYesNo(m_gui->getArkWidget(),
		i18n("The extract operation failed.\n"
		     "Do you wish to view the shell output?"), i18n("Error"));
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
	  KMessageBox::error(m_gui->getArkWidget(),
			     i18n("You probably don't have sufficient permissions.\n"
				  "Please check the file owner and the integrity "
				  "of the archive."));
	}
      else
	bSuccess = true;
    }
  else
    {
      QApplication::restoreOverrideCursor();
      int ret = KMessageBox::warningYesNo(m_gui->getArkWidget(),
		 i18n("The add operation failed.\n"
		      "Do you wish to view the shell output?"), i18n("Error"));
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


void Arch::slotReceivedTOC(KProcess*, char* _data, int _length)
{
  char c = _data[_length];
  _data[_length] = '\0';

  m_settings->appendShellOutputData( _data );

  int lfChar, startChar = 0;

  while(!m_finished)
  {
  	for(lfChar = startChar; _data[lfChar] != '\n' && lfChar < _length;
	    lfChar++);

	if(_data[lfChar] != '\n') break;	// We are done all the complete lines

	_data[lfChar] = '\0';
	m_buffer.append(_data + startChar);
	_data[lfChar] = '\n';
	startChar = lfChar + 1;

	if(m_headerString.isEmpty())
	{
		processLine(m_buffer);
	}
	else if( m_buffer.find(m_headerString) == -1 )
	{
		if( m_header_removed && !m_finished)
		{
			if(!processLine(m_buffer))
			{
				// Have faith - maybe it wasn't a header?
				m_header_removed = false;
				m_error = true;
			}
		}
	}
	else if(!m_header_removed)
		m_header_removed = true;
	else
		m_finished = true;

	m_buffer = "";
  }
  if(!m_finished)
	m_buffer.append(_data + startChar);	// Append what's left of the buffer

  _data[_length] = c;
}


bool Arch::processLine(const QCString &line)
{
  QString columns[11];
  unsigned int pos = 0;
  int strpos, len;

  // Go through our columns, try to pick out data, return silently on failure
  for(QPtrListIterator <ArchColumns>col(m_archCols); col.current(); ++col)
  {
  	ArchColumns *curCol = *col;

	strpos = curCol->pattern.search(line, pos);
	len = curCol->pattern.matchedLength();

	if(-1 == strpos || len >curCol->maxLength)
	{
		if(curCol->optional)
			continue;	// More?
		else
		{
			kdDebug(1601) << "processLine failed to match critical column"
				      << endl;
			return false;
		}
	}

	pos = strpos + len;

	columns[curCol->colRef] = line.mid(strpos, len);
  }


  if(m_dateCol >= 0)
  {
    QString year = m_repairYear >= 0?
	Utils::fixYear(columns[m_repairYear].ascii()) : columns[m_fixYear];
    QString month = m_repairMonth >= 0?
	QString("%1").arg(Utils::getMonth(columns[m_repairMonth].ascii())) :
	columns[m_fixMonth];
    QString timestamp= QString::fromLatin1("%1-%2-%3 %4")
      .arg(year)
      .arg(month)
      .arg(columns[m_fixDay])
      .arg(columns[m_fixTime]);

    columns[m_dateCol] = timestamp;
  }

  QStringList list;
  for (int i = 0; i < m_numCols; ++i)
    {
      list.append(columns[i]);
    }
  m_gui->listingAdd(&list); // send the entry to the GUI

  return true;
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


Arch *Arch::archFactory(ArchType aType, ArkSettings *settings,
			ArkWidgetBase *parent, const QString &filename)
{
	switch(aType)
	{
	case TAR_FORMAT: return new TarArch(settings, parent, filename);
	case ZIP_FORMAT: return new ZipArch(settings, parent, filename);
	case LHA_FORMAT: return new LhaArch(settings, parent, filename);
	case COMPRESSED_FORMAT: return new CompressedFile(settings,
							  parent, filename);
	case ZOO_FORMAT: return new ZooArch(settings, parent, filename);
	case RAR_FORMAT: return new RarArch(settings, parent, filename);
	case AA_FORMAT: return new ArArch(settings, parent, filename);
	case UNKNOWN_FORMAT:
	default: return 0;
	}
}

ArchType Arch::getArchType(const QString &archname, QString &extension,
                           const KURL &realURL)
{
  // Get the non-temporary name. Since we don't actually open anything
  // with it, the path information is irrelevant
  QString fileName = realURL.isEmpty() ? archname : realURL.fileName();

  ArchType extType = getArchTypeByExtension(fileName, extension);

  if (UNKNOWN_FORMAT == extType)
  {
    // now try using magic
    KMimeMagic *mimePtr = KMimeMagic::self();
    KMimeMagicResult * mimeResultPtr = mimePtr->findFileType(archname);
    QString mimetype = mimeResultPtr->mimeType();
    extension = QString::null;

    kdDebug(1601) << "The mime type is " << mimetype << endl;

    if (mimetype == "application/x-rar")
      extType = RAR_FORMAT;
    if (mimetype == "application/x-lha")
      extType = LHA_FORMAT;
    if (mimetype == "application/x-archive")
      extType = AA_FORMAT;
    if (mimetype == "application/x-tar")
      extType = TAR_FORMAT;
    if (mimetype == "application/x-zip")
      extType = ZIP_FORMAT;
  }

  return extType;
}


/**
* Try to determine the archive type by the extension. We separate
* this functionality out, to make it as easy to use as the MIME
* magic.
*/
ArchType Arch::getArchTypeByExtension(const QString &archname,
                                      QString &extension)
{
	if ((archname.right(4) == ".tgz") || (archname.right(4) == ".tzo")
	    || (archname.right(4) == ".taz")
	    || (archname.right(4) == ".tar"))
	{
		extension = archname.right(4);
		return TAR_FORMAT;
	}
	/* Patch by Matthias Belitz for handling KOffice docs */
	/*  if ((archname.right(4) == ".kil") || (archname.right(4) == ".kwd")
	     || (archname.right(4) == ".kwt")
	     || (archname.right(4) == ".ksp") || (archname.right(4) == ".kpr")
	     || (archname.right(4) == ".kpt"))
	{
		extension = archname.right(4);
		return TAR_FORMAT;
	}*/
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
	if ( ( archname.right( 4 ) == ".zip" ) ||
             ( archname.right( 4 ) == ".xpi" ) ||
             ( archname.right( 4 ) == ".exe" ) ||
             ( archname.right( 4 ) == ".EXE" ) )
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

	return UNKNOWN_FORMAT;
}


#include "arch.moc"



