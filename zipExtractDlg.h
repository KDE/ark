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

#ifndef ZIP_EXTRACT_DLG_H
#define ZIP_EXTRACT_DLG_H

#include "iostream.h"
#include <qwidget.h>

// KDE includes
#include <kfiledialog.h>

class ZipExtractDlg : public KFileBaseDialog {
	Q_OBJECT
public:
	ZipExtractDlg( QString dirName, QWidget *parent=0, const char *name=0 );

protected:
	QVBoxLayout *boxLayout;
	QGridLayout *lafBox;
	
	virtual void initGUI();
	virtual bool getShowFilter();
	virtual KFileInfoContents *initFileList( QWidget *parent );
	virtual QWidget *swallower() { cerr << "my swallower\n"; return this; }
};

#endif /* ZIP_EXTRACT_DLG_H */

