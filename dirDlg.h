//  -*-C++-*-           emacs magic for .h files
/*

 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org
 2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)

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

#ifndef DIRDLG_H
#define DIRDLG_H

// Qt includes
#include <qdialog.h>

// ark includes
#include "arksettings.h"

class QListBoxItem;
class QLabel;
class QLineEdit;
class QRadioButton;
class QButtonGroup;

enum { NUM_RADIOS = 3, NUM_DIR_TYPES = 4 };

class WidgetHolder
{
public:
  void hide();
  void show();
  friend class DirDlg;
private:
  QLabel *lDirType, *lHorizLine;
  QLineEdit *fixedLE;
  QButtonGroup *buttonGroup;
  QRadioButton *radios[NUM_RADIOS];
};

class DirDlg : public QDialog 
{
  Q_OBJECT
public:
  DirDlg(  ArkSettings *d, QWidget *parent=0, const char *name=0 );
  ~DirDlg();
  static QString getDirType(int item);
public slots:
  void getFavDir();	
  void getFixedDir();
  void saveConfig();
  void dirTypeChanged(int _dirType);
private: // methods  
  void initConfig();
  void createRepeatingWidgets();
  void hideWidgets();
private: // data
  ArkSettings *data;
  WidgetHolder *widgets[NUM_DIR_TYPES]; // pointers to the widgets
  QLineEdit *favLE;  // the favorite directory
  QListBox *pListBox;
};

#endif /* DIRDLG_H */

