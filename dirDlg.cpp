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
#include <qlineedit.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>

// KDE includes
#include <klocale.h>

// ark includes
#include "dirDlg.h"
#include "dirDlg.moc"


DirDlg::DirDlg( ArkData *d, QWidget *parent, const char *name )
	: QDialog( parent, name, true )
{
	data = d;
	
	QVBoxLayout *mainLayout = new QVBoxLayout( this, 10 );

	QHBoxLayout *hbl1 = new QHBoxLayout();
	mainLayout->addLayout( hbl1 );
	
	/**
	 * Build the Favorite directory selection
	 */
	QLabel *l1 = new QLabel( i18n("Favorite directory :"), this );
	l1->setFixedSize( l1->sizeHint() );
	hbl1->addWidget( l1 );
	
	QLineEdit *le = new QLineEdit( this );
	le->setFixedHeight( le->sizeHint().height() );
	le->setMinimumWidth( le->sizeHint().width() );
	hbl1->addWidget( le );
	
	QPushButton *browse = new QPushButton( i18n("Browse..."), this );
	browse->setFixedSize( browse->sizeHint() );
	hbl1->addWidget( browse );

	/**
	 * Build the 4 directories selection
	 */
	QHBoxLayout *hbl2 = new QHBoxLayout();
	mainLayout->addLayout( hbl2 );
	
	QVBoxLayout *vbl1 = new QVBoxLayout();
	QVBoxLayout *vbl2 = new QVBoxLayout();
	
	hbl2->addLayout( vbl1 );
	
	// *******
	// *@@*  *
	// *******
	// *  *  *
	// *******
	QButtonGroup *bg1 = new QButtonGroup( i18n("Open directory"), this );
	vbl1->addWidget( bg1 );
	
	QVBoxLayout *vblg1 = new QVBoxLayout( bg1, 10 );
	vblg1->addSpacing( 10 );
	
	QRadioButton *r1 = new QRadioButton( i18n("Favorite directory"), bg1 );
	r1->setFixedSize( r1->sizeHint() );
	vblg1->addWidget( r1, 0, AlignLeft );
	
	QRadioButton *r2 = new QRadioButton( i18n("Last open directory"), bg1 );
	r2->setFixedSize( r2->sizeHint() );
	vblg1->addWidget( r2, 0, AlignLeft );
	
	QRadioButton *r3 = new QRadioButton( i18n("Fixed : "), bg1 );
	r3->setFixedSize( r3->sizeHint() );
	QHBoxLayout *hblg1 = new QHBoxLayout();
	vblg1->addLayout( hblg1 );
	
	hblg1->addWidget( r3 );
	
	QLineEdit *le1 = new QLineEdit( bg1 );
	le1->setMinimumSize( le1->sizeHint() );
	hblg1->addWidget( le1 );

	QPushButton *browse1 = new QPushButton( i18n("..."), bg1 );
	browse1->setFixedSize( browse1->sizeHint() );
	hblg1->addWidget( browse1);

	
	// *******
	// *  *  *
	// *******
	// *@@*  *
	// *******
	QButtonGroup *bg2 = new QButtonGroup( i18n("Open directory"), this );
	vbl1->addWidget( bg2 );
	
	QVBoxLayout *vblg2 = new QVBoxLayout( bg2, 10 );
	vblg2->addSpacing( 10 );
	
	QRadioButton *r4 = new QRadioButton( i18n("Favorite directory"), bg2 );
	r4->setFixedSize( r4->sizeHint() );
	vblg2->addWidget( r4, 0, AlignLeft );
	
	QRadioButton *r5 = new QRadioButton( i18n("Last open directory"), bg2 );
	r5->setFixedSize( r5->sizeHint() );
	vblg2->addWidget( r5, 0, AlignLeft );
	
	QRadioButton *r6 = new QRadioButton( i18n("Fixed : "), bg2 );
	r6->setFixedSize( r6->sizeHint() );
	QHBoxLayout *hblg2 = new QHBoxLayout();
	vblg2->addLayout( hblg2 );
	
	hblg2->addWidget( r6 );
	
	QLineEdit *le2 = new QLineEdit( bg2 );
	le2->setMinimumSize( le2->sizeHint() );
	hblg2->addWidget( le2 );

	QPushButton *browse2 = new QPushButton( i18n("..."), bg2 );
	browse2->setFixedSize( browse2->sizeHint() );
	hblg2->addWidget( browse2 );


	
	// *******
	// *  *@@*
	// *******
	// *  *  *
	// *******
	QButtonGroup *bg3 = new QButtonGroup( i18n("Open directory"), this );
	vbl2->addWidget( bg3 );
	
	QVBoxLayout *vblg3 = new QVBoxLayout( bg3, 10 );
	vblg3->addSpacing( 10 );
	
	QRadioButton *r7 = new QRadioButton( i18n("Favorite directory"), bg3 );
	r7->setFixedSize( r7->sizeHint() );
	vblg3->addWidget( r7, 0, AlignLeft );
	
	QRadioButton *r8 = new QRadioButton( i18n("Last open directory"), bg3 );
	r8->setFixedSize( r8->sizeHint() );
	vblg3->addWidget( r8, 0, AlignLeft );
	
	QRadioButton *r9 = new QRadioButton( i18n("Fixed : "), bg3 );
	r9->setFixedSize( r9->sizeHint() );
	QHBoxLayout *hblg3 = new QHBoxLayout();
	vblg3->addLayout( hblg3 );
	
	hblg3->addWidget( r9 );
	
	QLineEdit *le3 = new QLineEdit( bg3 );
	le3->setMinimumSize( le3->sizeHint() );
	hblg3->addWidget( le3 );

	QPushButton *browse3 = new QPushButton( i18n("..."), bg3 );
	browse3->setFixedSize( browse3->sizeHint() );
	hblg3->addWidget( browse3 );


	
	// *******
	// *  *  *
	// *******
	// *  *@@*
	// *******
	QButtonGroup *bg4 = new QButtonGroup( i18n("Open directory"), this );
	vbl1->addWidget( bg4 );
	
	QVBoxLayout *vblg4 = new QVBoxLayout( bg4, 10 );
	vblg4->addSpacing( 10 );
	
	QRadioButton *r10 = new QRadioButton( i18n("Favorite directory"), bg4 );
	r10->setFixedSize( r10->sizeHint() );
	vblg4->addWidget( r10, 0, AlignLeft );
	
	QRadioButton *r11 = new QRadioButton( i18n("Last open directory"), bg4 );
	r11->setFixedSize( r11->sizeHint() );
	vblg4->addWidget( r11, 0, AlignLeft );
	
	QRadioButton *r12 = new QRadioButton( i18n("Fixed : "), bg4 );
	r12->setFixedSize( r12->sizeHint() );
	QHBoxLayout *hblg4 = new QHBoxLayout();
	vblg4->addLayout( hblg4 );
	
	hblg4->addWidget( r12 );
	
	QLineEdit *le4 = new QLineEdit( bg4 );
	le4->setMinimumSize( le4->sizeHint() );
	hblg4->addWidget( le4 );

	QPushButton *browse4 = new QPushButton( i18n("..."), bg4 );
	browse4->setFixedSize( browse4->sizeHint() );
	hblg4->addWidget( browse4 );
		
	
	// Build the OK/Cancel buttons layout
	QHBoxLayout *hbl = new QHBoxLayout();
	mainLayout->addLayout( hbl );
	hbl->addStretch( 1 );
	
	QPushButton *ok = new QPushButton( i18n("OK"), this );
	ok->setFixedSize( ok->sizeHint() );
	connect( ok, SIGNAL( clicked() ), SLOT( accept() ) );
	hbl->addWidget( ok );

	QPushButton *cancel = new QPushButton( i18n("Cancel"), this );
	cancel->adjustSize();
	connect( cancel, SIGNAL( clicked() ), SLOT( reject() ) );
	hbl->addWidget( cancel );

	mainLayout->activate();
	setFixedSize( sizeHint() );
}

/*
TestDlg::TestDlg( const QString& url, QWidget *parent )
	: QDialog( parent )
{
	KDirDialog *kdd = new KDirDialog( "/home/fx/tmp", this, "toto");

	QVBoxLayout *mainLayout = new QVBoxLayout( this );

	mainLayout->addWidget( kdd->swallower() );

	gb = new QGroupBox( i18n( "Add File Options" ), this );
	gb->setAlignment( AlignLeft );
	mainLayout->addWidget( gb );

	ok = new QPushButton( i18n("OK"), this );
	ok->adjustSize();
	mainLayout->addWidget( ok );

	connect( ok, SIGNAL( clicked() ), SLOT( accept() ) );
	cancel = new QPushButton( i18n("Cancel"), this );
	cancel->adjustSize();
	connect( cancel, SIGNAL( clicked() ), SLOT( reject() ) );
	mainLayout->addWidget( cancel );

	fullcb = new QCheckBox( i18n("Store Full Path"), gb );
	fullcb->adjustSize();
	mainLayout->addWidget( fullcb );
	
	updatecb = new QCheckBox( i18n("Only Add Newer Files") , gb );
	updatecb->adjustSize();
	mainLayout->addWidget( updatecb );

	mainLayout->activate();
}
*/
