/*

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

#ifndef DIRDLG_H
#define DIRDLG_H

// Qt includes
#include <qdialog.h>
#include <qlineedit.h>
#include <qradiobutton.h>

// ark includes
#include "arkdata.h"

class DirDlg : public QDialog {
	Q_OBJECT
public:
	DirDlg(  ArkData *d, QWidget *parent=0, const char *name=0 );
public slots:
	void getFavDir();	
	void getStartDir();	
	void getOpenDir();	
	void getExtractDir();	
	void getAddDir();	
	void favDirChanged(const QString &);
	void saveConfig();
private:
	ArkData *data;
	QLineEdit *favLE;
	QLineEdit *openLE;
	QLineEdit *startLE;
	QLineEdit *extractLE;
	QLineEdit *addLE;
	QRadioButton *r1, *r2, *r3, *r4, *r5, *r6, *r7, *r8, *r9, *r10, *r11, *r12;
	
	void initConfig();
};

#endif /* DIRDLG_H */

