/*

 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

 2002: Helio Chissini de Castro <helio@conectiva.com.br>
 2001: Corel Corporation (author: Michael Jarrett, michaelj@corel.com)
 1999-2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)
 1999: Francois-Xavier Duranceau duranceau@kde.org
 1997-1999: Rob Palmbos palm9744@kettering.edu
 2003: Hans Petter Bieker <bieker@kde.org>

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

// C includes
#include <stdlib.h>
#include <time.h>

#include <errno.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>
#include <config.h>

// for statfs:
#ifdef BSD4_4
#include <sys/mount.h>
#elif defined(__linux__)
#include <sys/vfs.h>
#elif defined(__sun)
#include <sys/statvfs.h>
#define STATFS statvfs
#elif defined(_AIX)
#include <sys/statfs.h>
#endif

#ifndef STATFS
#define STATFS statfs
#endif

// KDE includes
#include <kdebug.h>
#include <kmessagebox.h>
#include <klocale.h>

// Qt includes
#include <qfile.h>

#include "arkutils.h"

QString ArkUtils::getTimeStamp(const QString &_month,
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
      year.sprintf("%d", ArkUtils::getYear(nMonth, thisYear, thisMonth));
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

int ArkUtils::getMonth(const char *strMonth)
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

int ArkUtils::getYear(int theMonth, int thisYear, int thisMonth)
{
  int monthDiff = QABS(thisMonth - theMonth);
  if (monthDiff > 6)
    return (thisYear - 1);
  else
    return thisYear;
}

QString ArkUtils::fixYear(const char *strYear)
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
  strlcat(fourDigits, strYear, sizeof(fourDigits));
  return fourDigits;
}

bool
ArkUtils::haveDirPermissions( const QString &strFile )
{
  return ( access( QFile::encodeName( strFile ), W_OK ) == 0 );
}

bool
ArkUtils::diskHasSpace(const QString &dir, long size)
  // check if disk has enough space to accommodate (a) new file(s) of
  // the given size in the partition containing the given directory
{
  kdDebug( 1601 ) << "diskHasSpace() " << "dir: " << dir << "Size: " << size << endl;
  struct STATFS buf;
  if (STATFS(QFile::encodeName(dir), &buf) == 0)
  {
    double nAvailable = (double)buf.f_bavail * buf.f_bsize;
    if ( nAvailable < (double)size )
    {
      KMessageBox::error(0, i18n("You have run out of disk space."));
      return false;
    }
  }
  else
  {
    // something bad happened
    kdWarning( 1601 ) << "diskHasSpace() failed" << endl;
    // Q_ASSERT(0);
  }
  return true;
}

long
ArkUtils::getSizes(QStringList *list)
{
  long sum = 0;
  QString str;

  for ( QStringList::Iterator it = list->begin(); it != list->end(); ++it)
  {
    str = *it;
    QFile f(str.right(str.length()-5));
    sum += f.size();
  }
  return sum;
}

