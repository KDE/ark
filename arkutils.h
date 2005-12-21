//  -*-C++-*-           emacs magic for .h files
/*

 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org
 1999-2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)
 2001: Corel Corporation (author: Michael Jarrett, michaelj@corel.com)
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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#ifndef ARKUTILS_H
#define ARKUTILS_H

#include <qstring.h>

#include <kio/global.h>

class QStringList;

// various functions for massaging timestamps
namespace ArkUtils
{
  int getYear(int theMonth, int thisYear, int thisMonth);
  int getMonth(const char *strMonth);
  QString fixYear(const QString& strYear);

  QString getTimeStamp(const QString &month,
                       const QString &day,
                       const QString &year);
  bool haveDirPermissions(const QString &strFile);
  bool diskHasSpace(const QString &dir, KIO::filesize_t size);
  KIO::filesize_t getSizes(QStringList *list);
}

#endif
