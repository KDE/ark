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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// Qt includes
#include <qbuttongroup.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qradiobutton.h>

// KDE includes
#include <klocale.h>
#include <kpushbutton.h>

// ark includes
#include "deleteDlg.h"
#include "deleteDlg.moc"


DeleteDlg::DeleteDlg( bool _selEnabled, QWidget *_parent, const char *_name )
	: QDialog( _parent, _name, true )
{
	setCaption( i18n("Delete") );

	QVBoxLayout *mainLayout = new QVBoxLayout( this, 10 );

	QLabel *l1 = new QLabel( i18n("What do you want to delete?"), this );
	l1->setFixedSize( l1->sizeHint() );
	mainLayout->addWidget( l1, 0, AlignLeft );

	QButtonGroup *bg1 = new QButtonGroup( this );
	mainLayout->addWidget( bg1 );

	QVBoxLayout *vblg1 = new QVBoxLayout( bg1, 10 );
	vblg1->addSpacing( 10 );

	m_rbSelection = new QRadioButton( i18n("Selected files"), bg1 );
	m_rbSelection->setFixedSize( m_rbSelection->sizeHint() );
	vblg1->addWidget( m_rbSelection, 0, AlignLeft );
	m_rbSelection->setEnabled( _selEnabled );
	m_rbSelection->setChecked( _selEnabled );

	QHBoxLayout *hbl1 = new QHBoxLayout();
	vblg1->addLayout( hbl1 );

	m_rbPatterns = new QRadioButton( i18n("Files: "), bg1 );
	m_rbPatterns->setFixedSize( m_rbPatterns->sizeHint() );
	hbl1->addWidget( m_rbPatterns );
	m_rbPatterns->setChecked( !_selEnabled );

	m_lePatterns = new QLineEdit( bg1 );
	m_lePatterns->setFixedHeight( m_lePatterns->sizeHint().height() );
	m_lePatterns->setMinimumWidth( m_lePatterns->sizeHint().width() );
	hbl1->addWidget( m_lePatterns );
	connect( m_lePatterns, SIGNAL(textChanged(const QString &)), SLOT(onChange(const QString&)));

	QHBoxLayout *hbl = new QHBoxLayout();
	mainLayout->addLayout( hbl );

	hbl->addStretch( 1 );

	KPushButton *ok = new KPushButton( KStdGuiItem::ok(), this );
	ok->setFixedSize( ok->sizeHint() );
	ok->setDefault(true);
	connect( ok, SIGNAL( clicked() ), SLOT( accept() ) );
	hbl->addWidget( ok );

	KPushButton *cancel = new KPushButton( KStdGuiItem::cancel(), this );
	cancel->setFixedSize( cancel->sizeHint() );
	connect( cancel, SIGNAL( clicked() ), SLOT( reject() ) );
	hbl->addWidget( cancel );

	mainLayout->activate();
	setFixedSize( sizeHint() );
}

bool DeleteDlg::isSelectionChecked()
{
	return m_rbSelection->isChecked();
}

QString DeleteDlg::patterns()
{
	return m_lePatterns->text();
}

void DeleteDlg::onChange( const QString& text )
{
	m_rbPatterns->setChecked( true );
}

