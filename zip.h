//  -*-C++-*-           emacs magic for .h files
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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/


#ifndef ZIPARCH_H
#define ZIPARCH_H

class QString;
class QCString;
class QStringList;
class KProcess;

class Arch;
class ArkWidget;

class ZipArch : public Arch
{
  Q_OBJECT
public:
  ZipArch( ArkWidget *_gui, const QString & _fileName );
  virtual ~ZipArch() { }
	
  virtual void open();
  virtual void create();
	
  virtual void addFile( const QStringList & );
  virtual void addDir(const QString & _dirName);

  virtual void remove(QStringList *);
  virtual void unarchFile(QStringList *, const QString & _destDir="",
			  bool viewFriendly=false);
	
  void testIntegrity();

  enum AddMode { Update = 1, Freshen, Move };

protected slots:
  void slotIntegrityExited(KProcess*);
	
private:
  void setHeaders();
};

#endif /* ZIPARCH_H */
