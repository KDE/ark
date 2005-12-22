//  -*-C++-*-           emacs magic for .h files
/*

 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#ifndef ARARCH_H
#define ARARCH_H

class QString;
class QCString;
class QStringList;

class Arch;
class ArkWidget;

class ArArch : public Arch 
{
  Q_OBJECT
public:
  ArArch( ArkWidget *_gui,
	   const QString & _fileName );
  virtual ~ArArch() {}

  virtual void open();
  virtual void create();
	
  virtual void addFile( const QStringList & );
  virtual void addDir(const QString &) {} // never gets called

  virtual void remove(QStringList *);
  virtual void unarchFileInternal();

private:
  void setHeaders();
};

#endif /* ARARCH_H */
