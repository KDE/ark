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

// Qt includes
#include <qpushbutton.h>
#include <qlabel.h>
#include <qlayout.h>

// KDE includes
#include <klocale.h>

// ark includes
#include "waitDlg.h"
#include "waitDlg.moc"


WaitDlg::WaitDlg( QWidget *_parent, const char *_name, bool _modal, WFlags _f )
	: QDialog( _parent, _name, _modal, _f )
{
	setCaption( i18n("ark - Extracting...") );
	QVBoxLayout *mainLayout = new QVBoxLayout( this, 10 );

	QLabel *l1 = new QLabel( i18n("Please wait..."), this );
	l1->setFixedSize( l1->sizeHint() );
	mainLayout->addWidget( l1 );

	QPushButton *cancel = new QPushButton( i18n("Cancel"), this );
	cancel->setFixedSize( cancel->sizeHint() );
	connect( cancel, SIGNAL( clicked() ), SLOT( onCancel() ) );
	mainLayout->addWidget( cancel );

	mainLayout->activate();
	setFixedSize( sizeHint() );
}


void WaitDlg::onCancel()
{
	emit( dialogClosed() );
	reject();
}

void WaitDlg::close()
{
	reject();
}

