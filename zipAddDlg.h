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
#include <qlayout.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qwidget.h>

// KDE includes
#include <kfiledialog.h>

// ark includes
#include "arkdata.h"


class ZipAddDlg : public KFileBaseDialog {
	Q_OBJECT
public:
	ZipAddDlg( ArkData*, QString, QWidget *parent=0, const char *name=0 );
	
protected:
	QVBoxLayout *boxLayout;
	QGridLayout *lafBox;
	QCheckBox *c1, *c2, *c3, *c4;
	QPushButton *m_bAdd, *m_bClose;
	QLineEdit *m_leNames;
	
	virtual void initGUI();
	virtual bool getShowFilter();
	virtual KFileInfoContents *initFileList( QWidget *parent );
	virtual QWidget *swallower() { return this; }
	
	void saveConfig();

	bool m_addClicked;	
	ArkData *m_data;
	
protected slots:
	void onAdd();	
	void onClose();
	void onHelp();
	
	void slotSelectionChanged(const QString&);
	void slotFileHighlighted(const QString&);
};

#endif /* ZIP_ADD_DLG_H */

