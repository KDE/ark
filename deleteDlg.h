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

#ifndef DELETE_DLG_H
#define DELETE_DLG_H

class QWidget;
class QDialog;
class QString;
class QLineEdit;
class QRadioButton;

class DeleteDlg : public QDialog {
	Q_OBJECT
public:
	DeleteDlg( bool, QWidget *parent=0, const char *name=0 );
	QString patterns();
	bool isSelectionChecked();

private:
	QLineEdit *m_lePatterns;
	QRadioButton *m_rbSelection, *m_rbPatterns;
	
private slots:
	void onChange( const QString & );
};

#endif /* DELETE_DLG_H */

