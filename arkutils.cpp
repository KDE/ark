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
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <config.h>

// C includes
#include <stdlib.h>
#include <time.h>

// KDE includes
#include <kdebug.h>

#include "utils.h"

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
  strlcat(fourDigits, strYear, sizeof(fourDigits));
  return fourDigits;
}
