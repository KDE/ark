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

#ifndef SHELL_OUTPUT_DLG_H
#define SHELL_OUTPUT_DLG_H

// Qt includes
#include <qdialog.h>
#include <qmultilineedit.h>

// ark includes
#include "arksettings.h"


class ShellOutputWidget : public QMultiLineEdit {
public:
	ShellOutputWidget( QDialog* _d ) : QMultiLineEdit( _d ) 
		{ setNumCols(80); };
};



class ShellOutputDlg : public QDialog {
	Q_OBJECT
public:
	ShellOutputDlg( ArkData *, QWidget *parent=0, const char *name=0 );
	
};

#endif /* SHELL_OUTPUT_DLG_H */

