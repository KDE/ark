//  -*-C++-*-           emacs magic for .h files
/*

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


#ifndef ARCH_H
#define ARCH_H

// KDE includes
#include <kprocess.h>

// Qt includes
#include <qstring.h>

// ark includes
#include "arksettings.h"
#include "filelistview.h"
class KProcess;
class Viewer;

// The following class is the base class for all of the archive types.
// In order for it to work properly with the KProcess, you have to
// connect the ProcessExited signal appropriately before spawning
// the core operations. Then the signal that the process exited can
// be intercepted by the viewer (in ark, ArkWidget) and dealt with
// appropriately. See LhaArch or ZipArch for a good model. Don't use
// TarArch or CompressedFile as models - they're too complicated!
//
//
// To add a new archive:
// 1. Create a new header file and a source code module
// 2. Add an entry to the ArchType enum in arkwidget.h.
// 3. Include your new header file in arkwidget.cc.
// 4. Add new cases for your format in createArchive(const QString &),
//   openArchive(const QString &) and ArkWidget::getArchType().
// 5. Add your extension to the list of valid archives in 
//   ArkSettings::getFilter (you might also want to add a separate entry)
//

class Arch : public QObject
{
  Q_OBJECT
public:
  Arch( ArkSettings *_settings, Viewer *_viewer, const QString & _fileName );
  virtual ~Arch();
	
  virtual void open() = 0;
  virtual void create() = 0;
  virtual void remove(QStringList *) = 0;

  virtual void addFile(QStringList *) = 0;
  virtual void addDir(const QString &) = 0;

  // unarch the files in the list or all files if the list is empty.
  // if _destDir is empty, look at settings for extract directory
  virtual void unarchFile(QStringList *, const QString & _destDir="") = 0;

  QString fileName() const { return m_filename; };
	
  enum EditProperties{ Add = 1, Delete = 2, Extract = 4,
    View = 8, Integrity = 16 };

  bool stderrIsError();

  // is the archive readonly?
  bool isReadOnly() { return m_bReadOnly; }

  void setReadOnly(bool bVal) { m_bReadOnly = bVal; }

protected slots:
  void slotCancel();
  void slotStoreDataStdout(KProcess*, char*, int);
  void slotStoreDataStderr(KProcess*, char*, int);
  void slotOpenExited(KProcess*);
	
  void slotExtractExited(KProcess*);
  void slotDeleteExited(KProcess*);
  void slotAddExited(KProcess*);

  void slotReceivedOutput(KProcess *, char*, int);

signals:
  void sigOpen( Arch *, bool, const QString &, int );
  void sigCreate( Arch *, bool, const QString &, int );
  void sigDelete(bool);
  void sigExtract(bool);
  void sigAdd(bool);
	
protected:  // data
  QString m_filename;
  QString m_shellErrorData;
  char m_buffer[1024];
  ArkSettings *m_settings;
  Viewer *m_gui;
  bool m_bReadOnly; // for readonly archives

  // lets tar delete unsuccessfully before adding without confusing the user
  bool m_bNotifyWhenDeleteFails; 

};

namespace Utils
{
  int getYear(int theMonth, int thisYear, int thisMonth);
  int getMonth(const char *strMonth);
  QString fixYear(const char *strYear);
  
  QString getTimeStamp(const QString &month,
		       const QString &day,
		       const QString &year);
}

// Column header strings

#define FILENAME_STRING i18n(" Filename ")
#define PERMISSION_STRING i18n(" Permissions ")
#define OWNER_GROUP_STRING i18n(" Owner/Group ")
#define SIZE_STRING i18n(" Size ")
#define TIMESTAMP_STRING i18n(" Timestamp ")
#define LINK_STRING i18n(" Link ")
#define PACKED_STRING i18n(" Size Now ")
#define RATIO_STRING i18n(" Ratio ")
#define CRC_STRING i18n(" CRC ")
#define METHOD_STRING i18n(" Method ")
#define VERSION_STRING i18n(" Version ")
#define OWNER_STRING i18n(" Owner ")
#define GROUP_STRING i18n(" Group ")





#endif /* ARCH_H */
