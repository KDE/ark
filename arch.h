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


enum ArchType {UNKNOWN_FORMAT, ZIP_FORMAT, TAR_FORMAT, AA_FORMAT,
	       LHA_FORMAT, RAR_FORMAT, ZOO_FORMAT, COMPRESSED_FORMAT};

ArchType getArchType( const QString & archname, QString &extension);

// The following class is the base class for all of the archive types.
// In order for it to work properly with the KProcess, you have to
// connect the ProcessExited signal appropriately before spawning
// the core operations. Then the signal that the process exited can
// be intercepted by the viewer (in ark, ArkWidget) and dealt with
// appropriately. See LhaArch or ZipArch for a good model. Don't use
// TarArch or CompressedFile as models - they're too complicated!
//
// Don't forget to set m_archiver_program and m_unarchiver_program
// and add a call to 
//     verifyUtilityIsAvailable(m_archiver_program, m_unarchiver_program);
// in the constructor of your class. It's OK to leave out the second argument.
//
// To add a new archive:
// 1. Create a new header file and a source code module
// 2. Add an entry to the ArchType enum in arch.h.
// 3. Include your new header file in arkwidget.cc.
// 4. Add new cases for your format in 
//    ArkWidget::createArchive(const QString &),
//    ArkWidget::openArchive(const QString &)
// and
//    getArchType() in arch.cpp
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
  // viewFriendly forces certain options like directory junking required by view/edit
  virtual void unarchFile(QStringList *, const QString & _destDir="",
			  bool viewFriendly=false) = 0;

  QString fileName() const { return m_filename; };

  enum EditProperties{ Add = 1, Delete = 2, Extract = 4,
    View = 8, Integrity = 16 };

  bool stderrIsError();

  // is the archive readonly?
  bool isReadOnly() { return m_bReadOnly; }

  void setReadOnly(bool bVal) { m_bReadOnly = bVal; }

  // check to see if the utility exists in the PATH of the user
  void verifyUtilityIsAvailable(const QString &,
				const QString & = QString::null);

  bool utilityIsAvailable() { return m_bUtilityIsAvailable; }

  QString getUtility() { return m_archiver_program; }

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

  // set to whether the archiving utility/utilities is/are in the user's PATH
  bool m_bUtilityIsAvailable;

  QString m_archiver_program;
  QString m_unarchiver_program;
};

// various functions for massaging timestamps
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
// don't forget to change common_texts.cpp if you change something here
#define FILENAME_STRING i18n(" Filename ")
#define PERMISSION_STRING i18n(" Permissions ")
#define OWNER_GROUP_STRING i18n(" Owner/Group ")
#define SIZE_STRING i18n(" Size ")
#define TIMESTAMP_STRING i18n(" Timestamp ")
#define LINK_STRING i18n(" Link ")
#define PACKED_STRING i18n(" Size Now ")
#define RATIO_STRING i18n(" Ratio ")
#define CRC_STRING i18n("acronym for Cyclic Redundancy Check"," CRC ")
#define METHOD_STRING i18n(" Method ")
#define VERSION_STRING i18n(" Version ")
#define OWNER_STRING i18n(" Owner ")
#define GROUP_STRING i18n(" Group ")

#endif /* ARCH_H */
