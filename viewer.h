//  -*-C++-*-           emacs magic for .h files
/*

 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

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
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

// This file contains a class called Viewer, which exists to help
// separate the archive classes in the backend from the GUI front-end.
// It passes on all its messages to the ArkWidget class for this 
// implementation.
//
// If you need to use the classes, just replace in your own viewer.
//
#ifndef __CVIEWER__H__
#define __CVIEWER__H__

#include <qstringlist.h>
#include "arkwidget.h"

class Viewer
{
public:
  Viewer(ArkWidget * _gui)
    : mGui(_gui) {}
  void add(QStringList *_entries) { mGui->listingAdd(_entries); }
  void setHeaders(QStringList *_headers,
		  int * _rightAlignCols, int _numColsToAlignRight)
    { mGui->setHeaders(_headers, _rightAlignCols, _numColsToAlignRight); }
  int getNumFilesInArchive() { return mGui->getNumFilesInArchive(); }
  int getCol(const QString & _columnHeader)
    { return mGui->getCol(_columnHeader); }
  QString getColData(const QString & _filename, int _col)
    { return mGui->getColData(_filename, _col); }
  
private:
  ArkWidget *mGui;
};

#endif //  __CVIEWER__H__
