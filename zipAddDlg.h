// OBSOLETE

/*

 $Id $

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org

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

#ifndef ZIP_ADD_DLG_H
#define ZIP_ADD_DLG_H

// Qt includes
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qwidget.h>

// KDE includes
#include <kfiledialog.h>
#include <kfileinfocontents.h>
// ark includes
#include "arksettings.h"
#include "zip.h"


class ZipAddDlg : public KFileDialog 
{
  Q_OBJECT
public:
  ZipAddDlg(ZipArch*, ArkSettings*, QString, QWidget *parent=0,
	    const char *name=0);
	
protected:
  QVBoxLayout *boxLayout;
  QGridLayout *lafBox;
  QCheckBox *c1, *c2, *c3, *c4;
  QComboBox *cb1, *cb2;
  QPushButton *m_bAdd, *m_bClose;
  QLineEdit *m_leNames;
  
  virtual void initGUI();
  virtual bool getShowFilter();
  virtual KFileInfoContents *initFileList( QWidget *parent );
  virtual QWidget *swallower() { return this; }
  
  void saveConfig();
  QString location();
  int mode();
  QString compression();
  
  bool m_addClicked;	
  ArkSettings *m_settings;
  ZipArch *m_zip;
  
protected slots:
  void onAdd();	
  void onClose();
  void onHelp();
 
  void slotSelectionChanged(const QString&);
  void slotFileHighlighted(const QString&);
  void slotFileSelected(const QString&);
};

#endif /* ZIP_ADD_DLG_H */

