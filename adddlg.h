//  -*-C++-*-           emacs magic for .h files
/*                                                                               ark -- archiver for the KDE project
 
 Copyright (C)
 
 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org
 1999-2000: Corel Corporation (author: Emily Ezust emilye@corel.com)
 
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

#ifndef __ADDDLG_H__
#define __ADDDLG_H__

#include <kdialogbase.h>
#include <qstringlist.h>
#include "arkwidget.h"  // for ArchType

class QLineEdit;
class QCheckBox;
class ArkSettings;
class KDirOperator;

class AddDlg : public KDialogBase 
{
  Q_OBJECT
public:
  AddDlg(ArchType _archtype, const QString & _sourceDir, 
	 ArkSettings *_settings, QWidget *parent=0, const char *name=0);

  ~AddDlg() { delete m_fileList;}
  QStringList *getFiles() { return m_fileList; }
public slots:
  void accept();
private: // methods
  void setupFirstTab();
  void setupSecondTab();

private: // data
  QString m_sourceDir;  
  KDirOperator *m_dirList;
  ArchType m_archtype;
  ArkSettings *m_settings;
  QStringList *m_fileList;
  // advanced options

  // zip:
  QCheckBox *m_cbRecurse, *m_cbJunkDirNames, *m_cbForceMS, *m_cbConvertLF2CRLF;

  // tar
  QCheckBox *m_cbReplaceOnlyNewer;
};


#endif //  __ADDDLG_H__
