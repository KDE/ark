//  -*-C++-*-           emacs magic for .h files
/*

    ark: A program for modifying archives via a GUI.

    Copyright (C)

    2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)
    2001: Corel Corporation (author: Michael Jarrett, michaelj@corel.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#ifndef __COMPRESSED_FILE_H__
#define __COMPRESSED_FILE_H__

class QString;
class QCString;
class QStringList;
class KProcess;
class KTempDir;

class Arch;
class ArkWidget;

//
// This isn't *really* an archive, but having this class in the program
// allows people to manage gzipped files if they want. If someone tries to
// add a file, the gz-file must be empty (hozat? it could be empty by having
// just been created). Otherwise, the user will be asked to create a REAL 
// archive, and then the added file(s) and the original file will be 
// transferred to the new archive.
//
class CompressedFile : public Arch
{
  Q_OBJECT
public:
  CompressedFile( ArkWidget *_gui, const QString & _fileName, const QString &_openAsMimeType );
  virtual ~CompressedFile();
	
  virtual void open();
  virtual void create();
	
  virtual void addFile( QStringList* );
  virtual void addDir(const QString &) { }

  virtual void remove(QStringList *);
  virtual void unarchFile(QStringList *, const QString & _destDir="",
			  bool viewFriendly=false);

  QString tempFileName(){ return m_tmpfile; }

private slots:
  void slotUncompressDone(KProcess *);
  void slotAddInProgress(KProcess*, char*, int);
  void slotAddDone(KProcess*);

private:
  void initExtract( bool, bool, bool );
  void setHeaders();
  void initData();
  QString extension();

  QString m_openAsMimeType;
  KTempDir * m_tempDirectory;
  QString m_tmpdir;
  QString m_tmpfile;
  QStringList m_defaultExtensions;

  // for use with addFile
  FILE *fd;

};



#endif // __COMPRESSED_FILE_H__
