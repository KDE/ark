/*

 $Id$

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

#ifndef SELECT_DLG_H
#define SELECT_DLG_H

// Qt includes
#include <qdialog.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qstring.h>

// ark includes
#include <arkdata.h>

class SelectDlg : public QDialog {
	Q_OBJECT
public:
	SelectDlg(  ArkData *d, QString _pattern, QWidget *parent=0, const char *name=0 );
	QString getRegExp() const;


private:
	ArkData *m_data;
	QLineEdit *m_regExp;
	QPushButton *m_ok;
	
private slots:
	void regExpChanged(const QString& _exp);
	void saveConfig();
};

#endif /* SELECT_DLG_H */

