//  -*-C++-*-           emacs magic for .h files
/*

 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org
 1999-2000: Corel Corporation (author: Emily Ezust emilye@corel.com)
 2001: Corel Corporation (author: Michael Jarrett, michaelj@corel.com)
<<<<<<< extractdlg.h
 2001: Roberto Selbach Teixeira (maragato@conectiva.com)

=======

>>>>>>> 1.19
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

#include <qradiobutton.h>

#include "arch.h"

class QWidget;
class QString;
class QStringList;
class QLineEdit;
class QComboBox;
class QDialog;
class KDialogBase;
class KHistoryCombo;
class KCompletion;
class KURLCompletion;

class ArkSettings;


class ExtractDlg : public KDialogBase
{
  Q_OBJECT
public:
  ExtractDlg(ArkSettings *_settings);
  ~ExtractDlg();
  enum ExtractOp{ All, Selected, Current, Pattern };
  int extractOp();
  void disableSelectedFilesOption();
  void disableCurrentFileOption() { m_radioCurrent->setEnabled(false); }
public slots:
  void browse();
  void choosePattern() { m_radioPattern->setChecked(true); }
  void openPrefs();
  void accept();

signals:
  // This signal is caught by ArkWidget, which selects all files matching
  // that pattern.
  void pattern(const QString &);

private: // data
  QRadioButton *m_radioCurrent, *m_radioAll, *m_radioSelected, *m_radioPattern;
  QLineEdit *m_patternLE;
  KHistoryCombo *m_extractDirCB;
  ArkSettings *m_settings;
};

// this class is used for displaying the filenames that were not
// extracted, if that number is greater than 1.

class ExtractFailureDlg : public QDialog
{
public:
  ExtractFailureDlg(QStringList *list,
		    QWidget *parent=0, char *name=0);
};

#endif //  __EXTRACTDLG_H__
