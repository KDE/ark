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

#ifndef ZIP_EXTRACT_DLG_H
#define ZIP_EXTRACT_DLG_H

// Qt includes
#include <qcheckbox.h>
#include <qlineedit.h>
#include <qradiobutton.h>
#include <qwidget.h>

// KDE includes
#include <kfiledialog.h>

// ark includes
#include "arksettings.h"


class ZipExtractDlg : public KFileDialog {
	Q_OBJECT
public:
	ZipExtractDlg( ArkData*, bool, QString, QWidget *parent=0, const char *name=0 );
	
	bool overwrite();
	bool lowerCase();
	bool junkPaths();
	
	QString getDestination() const;
	
	enum selectionType{ All, Selection, Pattern };
	int selection();
	QString pattern();
		
protected:
	QVBoxLayout *boxLayout;
	QGridLayout *lafBox;
	QRadioButton *r1, *r2, *r3;
	QCheckBox *r4, *r5, *r6;
	QLineEdit *le1;
	
	bool m_selection;
	
	virtual void initGUI();
	virtual bool getShowFilter();
	virtual KFileInfoContents *initFileList( QWidget *parent );
	virtual QWidget *swallower() { return this; }
	
	void saveConfig();
	
	ArkData *m_data;
	
protected slots:
	void onExtract();
	void onPatternChanged(const QString &);	
};

#endif /* ZIP_EXTRACT_DLG_H */

