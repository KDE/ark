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

#ifndef __EXTRACTDLG_H__
#define __EXTRACTDLG_H__

#include <qtabdialog.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include "arkwidget.h"  // for ArchType

class QLineEdit;
class ArkSettings;

class ExtractDlg : public QTabDialog 
{
  Q_OBJECT
public:
  ExtractDlg(ArchType _archtype, ArkSettings *_settings);
  enum ExtractOp{ All, Selected, Current, Pattern };
  int extractOp();
public slots:
  void browse();
  void choosePattern() { m_radioPattern->setChecked(true); }
  void accept();
signals:
  // This signal is caught by ArkWidget, which selects all files matching
  // that pattern.
  void pattern(const QString &); 

private: // methods
  void setupFirstTab();
  void setupSecondTab();

private: // data
  QRadioButton *m_radioCurrent, *m_radioAll, *m_radioSelected, *m_radioPattern;
  QLineEdit *m_patternLE;
  QLineEdit *m_extractDirLE;
  ArkSettings *m_settings;
  ArchType m_archtype;
  // advanced options 
  QCheckBox *m_cbOverwrite, *m_cbPreservePerms, *m_cbToLower, *m_cbToUpper;
  QCheckBox *m_cbDiscardPathnames;
};


#endif //  __EXTRACTDLG_H__
