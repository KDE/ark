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
#include <qbuttongroup.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qregexp.h>

// KDE includes
#include <klocale.h>

// ark includes
#include "selectDlg.h"
#include "selectDlg.moc"

#define BROWSE_WIDTH 40

SelectDlg::SelectDlg( ArkData *_data, QString _pattern, QWidget *_parent, const char *_name )
	: QDialog( _parent, _name, true )
{
	m_data = _data;
	
	setCaption( i18n("ark - Selection") );
	QVBoxLayout *mainLayout = new QVBoxLayout( this, 10 );

	/**
	 * Tar command horizontal layout
	 */
	QHBoxLayout *hbl1 = new QHBoxLayout();
	mainLayout->addLayout( hbl1 );
	
	QLabel *l1 = new QLabel( i18n("Select files:"), this );
	l1->setFixedSize( l1->sizeHint() );
	hbl1->addWidget( l1 );

	m_ok = new QPushButton( i18n("OK"), this );	

	m_regExp = new QLineEdit( this );
	m_regExp->setFixedSize( m_regExp->sizeHint() );
	m_regExp->setText( _pattern );
	m_regExp->setSelection(0, _pattern.length() );
	regExpChanged( _pattern );
	hbl1->addWidget( m_regExp );
	connect( m_regExp, SIGNAL(textChanged(const QString&)), SLOT(regExpChanged(const QString&)) );
	
	QHBoxLayout *hbl = new QHBoxLayout();
	mainLayout->addStretch( 1 );
	mainLayout->addLayout( hbl );
	
	hbl->addStretch( 1 );
	m_ok->setFixedSize( m_ok->sizeHint() );
	m_ok->setDefault(true);
	connect( m_ok, SIGNAL( clicked() ), SLOT( saveConfig() ) );
	hbl->addWidget( m_ok );

	QPushButton *cancel = new QPushButton( i18n("Cancel"), this );
	cancel->setFixedSize( cancel->sizeHint() );
	connect( cancel, SIGNAL( clicked() ), SLOT( reject() ) );
	hbl->addWidget( cancel );

	mainLayout->activate();
	setFixedSize( sizeHint() );
	m_regExp->setFocus();
}


void SelectDlg::saveConfig()
{
	if( !m_regExp->text().isEmpty() )
		m_data->setTarCommand( m_regExp->text() );
	accept();
}

void SelectDlg::regExpChanged(const QString& _exp)
{
	QRegExp reg_exp(_exp, true, true);
	if(reg_exp.isValid())
		m_ok->setEnabled(true);
	else
		m_ok->setEnabled(false);
}

QString SelectDlg::getRegExp() const
{
	return m_regExp->text();
}
