//  -*-C++-*-           emacs magic for .h files
/*

 $Id $

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org

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


#ifndef ARCH_H
#define ARCH_H

// Qt includes
#include <qstring.h>

// ark includes
#include "arksettings.h"
#include "filelistview.h"

class Viewer;

class Arch : public QObject
{
  Q_OBJECT
public:
  Arch( ArkSettings *_settings, Viewer *_viewer, const QString & _fileName )
    : m_filename(_fileName), m_settings(_settings), m_gui(_viewer) {}

  virtual ~Arch() {};
	
  virtual void open() = 0;
  virtual void create() = 0;
  virtual void remove(QStringList *) = 0;

  virtual int addFile(QStringList *) = 0;

  // unarch the files in the list or all files if the list is empty
  virtual QString unarchFile(QStringList *) = 0;

  virtual int actionFlag() = 0;
	
  QString fileName() const { return m_filename; };
  FileListView *fileList() { return 0; } //m_arkwidget->fileList(); }
	
  enum EditProperties{ Add = 1, Delete = 2, Extract = 4,
    View = 8, Integrity = 16 };

signals:
  void sigOpen( bool, const QString &, int );
  void sigCreate( bool, const QString &, int );
	
protected:
  QString m_filename;
  QString m_shellErrorData;
  char m_buffer[1024];
  ArkSettings *m_settings;
  //  ArkWidget *m_arkwidget;
  Viewer *m_gui;
};


enum AddRetCodes { SUCCESS, FAILURE, UNSUPDIR } ;

#endif /* ARCH_H */
