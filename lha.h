//  -*-C++-*-           emacs magic for .h files
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
#ifndef LHAARCH_H
#define LHAARCH_H

// Qt includes
#include <qstring.h>
#include <qstrlist.h>

#include "arch.h"

class LhaArch : public Arch 
{
  Q_OBJECT
public:
  LhaArch( ArkSettings *_settings, Viewer *_gui,
	   const QString & _fileName );
  virtual ~LhaArch() { }
	
  virtual void open();
  virtual void create();
	
  virtual void addFile( QStringList* );
  virtual void addDir(const QString & _dirName);

  virtual void remove(QStringList *);
  virtual void unarchFile(QStringList *, const QString & _destDir="");

protected:
  bool m_header_removed, m_finished, m_error;

protected slots:
  void slotReceivedTOC(KProcess *, char *, int);
	
private:
  QString m_archiver_program;
  void processLine( char* );	
  void initExtract( bool, bool, bool );
  void setHeaders();
};

#endif /* ARCH_H */
