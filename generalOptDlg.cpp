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

// Qt includes
#include <qlayout.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qbuttongroup.h>

// KDE includes
#include <klocale.h>

// ark includes
#include "generalOptDlg.h"
#include "generalOptDlg.moc"

#define BROWSE_WIDTH 40

GeneralDlg::GeneralDlg( ArkSettings *_d, QWidget *_parent, const char *_name )
	: QDialog( _parent, _name, true )
{
	m_settings = _d;
	
	setCaption( i18n("ark - General preferences") );
	QVBoxLayout *mainLayout = new QVBoxLayout( this, 10 );

	/**
	 * Tar command horizontal layout
	 */
	QHBoxLayout *hbl1 = new QHBoxLayout();
	mainLayout->addLayout( hbl1 );
	
	QLabel *l1 = new QLabel( i18n("GNU Tar command:"), this );
	l1->setFixedSize( l1->sizeHint() );
	hbl1->addWidget( l1 );
	
	tarLE = new QLineEdit( this );
	tarLE->setFixedSize( tarLE->sizeHint() );
	hbl1->addWidget( tarLE );
	connect( tarLE, SIGNAL(textChanged(const QString&)), SLOT(tarChanged(const QString&)) );
	
	QHBoxLayout *hbl = new QHBoxLayout();
	mainLayout->addStretch( 1 );
	mainLayout->addLayout( hbl );
	
	hbl->addStretch( 1 );
	ok = new QPushButton( i18n("OK"), this );	
	ok->setFixedSize( ok->sizeHint() );
	ok->setDefault(true);
	connect( ok, SIGNAL( clicked() ), SLOT( saveConfig() ) );
	hbl->addWidget( ok );

	QPushButton *cancel = new QPushButton( i18n("Cancel"), this );
	cancel->setFixedSize( cancel->sizeHint() );
	connect( cancel, SIGNAL( clicked() ), SLOT( reject() ) );
	hbl->addWidget( cancel );

	initConfig();
	
	mainLayout->activate();
	setFixedSize( sizeHint() );
}

void GeneralDlg::initConfig()
{
	tarLE->setText( m_settings->getTarCommand() );
}

void GeneralDlg::saveConfig()
{
	m_settings->setTarCommand( tarLE->text() );
	accept();
}

void GeneralDlg::tarChanged(const QString& _cmd)
{
	ok->setEnabled( !_cmd.isEmpty() );
}
