//  -*-C++-*-           emacs magic for .h files
/*

 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

 2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)
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
#ifndef __ZOO_H__
#define __ZOO_H__ 

class QString;
class QCString;
class QStringList;

class Arch;
class ArkSettings;
class ArkWidget;

class ZooArch : public Arch
{
  Q_OBJECT
public:
  ZooArch( ArkSettings *_settings, ArkWidget *_gui,
	   const QString & _fileName );
  virtual ~ZooArch() { }
	
  virtual void open();
  virtual void create();
	
  virtual void addFile( QStringList* );
  virtual void addDir(const QString & _dirName);

  virtual void remove(QStringList *);
  virtual void unarchFile(QStringList *, const QString & _destDir="",
			  bool viewFriendly=false);

protected slots:
  virtual bool processLine(const QCString &line);
	
private:
  void initExtract( bool, bool, bool );
  void setHeaders();
};

#endif /*  __ZOO_H__ */
