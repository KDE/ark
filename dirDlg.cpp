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
#include <kfiledialog.h>
#include <klocale.h>

// ark includes
#include "dirDlg.h"
#include "dirDlg.moc"

#define BROWSE_WIDTH 40

DirDlg::DirDlg( ArkData *d, QWidget *parent, const char *name )
	: QDialog( parent, name, true )
{
	data = d;
	
	setCaption( i18n("ark - Directories preferences") );
	QVBoxLayout *mainLayout = new QVBoxLayout( this, 10 );

	QHBoxLayout *hbl1 = new QHBoxLayout();
	mainLayout->addLayout( hbl1 );
	
	/**
	 * Build the Favorite directory selection
	 */
	QLabel *l1 = new QLabel( i18n("Favorite directory:"), this );
	l1->setFixedSize( l1->sizeHint() );
	hbl1->addWidget( l1 );
	
	favLE = new QLineEdit( this );
	favLE->setFixedHeight( favLE->sizeHint().height() );
	favLE->setMinimumWidth( favLE->sizeHint().width() );
	hbl1->addWidget( favLE );
	connect( favLE, SIGNAL(textChanged(const QString&)), SLOT(favDirChanged(const QString&)) );
	
	QPushButton *browse = new QPushButton( i18n("..."), this );
	browse->setFixedHeight( browse->sizeHint().height() );
	browse->setFixedWidth( BROWSE_WIDTH );
	hbl1->addWidget( browse );
        connect( browse, SIGNAL(clicked()), SLOT(getFavDir()) );
	/**
	 * Build the 4 directories selection
	 */
	QHBoxLayout *hbl2 = new QHBoxLayout();
	mainLayout->addLayout( hbl2 );
	
	QVBoxLayout *vbl1 = new QVBoxLayout();
	QVBoxLayout *vbl2 = new QVBoxLayout();
	
	hbl2->addLayout( vbl1 );
	hbl2->addLayout( vbl2 );
	
	// *******
	// *@@*  *
	// *******
	// *  *  *
	// *******
	QButtonGroup *bg1 = new QButtonGroup( i18n("Start-up directory"), this );
	vbl1->addWidget( bg1 );
	
	QVBoxLayout *vblg1 = new QVBoxLayout( bg1, 10 );
	vblg1->addSpacing( 10 );
	
	r1 = new QRadioButton( i18n("Favorite directory"), bg1 );
	r1->setFixedSize( r1->sizeHint() );
	vblg1->addWidget( r1, 0, AlignLeft );
	
	r2 = new QRadioButton( i18n("Last open directory"), bg1 );
	r2->setFixedSize( r2->sizeHint() );
	vblg1->addWidget( r2, 0, AlignLeft );
	
	r3 = new QRadioButton( i18n("Fixed:"), bg1 );
	r3->setFixedSize( r3->sizeHint() );
	QHBoxLayout *hblg1 = new QHBoxLayout();
	vblg1->addLayout( hblg1 );
	
	hblg1->addWidget( r3 );
	
	startLE = new QLineEdit( bg1 );
	startLE->setMinimumSize( startLE->sizeHint() );
	hblg1->addWidget( startLE );

	QPushButton *browse1 = new QPushButton( i18n("..."), bg1 );
	browse1->setFixedHeight( browse1->sizeHint().height() );
	browse1->setFixedWidth( BROWSE_WIDTH );
	hblg1->addWidget( browse1);
        connect( browse1, SIGNAL(clicked()), SLOT(getStartDir()) );

	
	// *******
	// *  *  *
	// *******
	// *@@*  *
	// *******
	QButtonGroup *bg2 = new QButtonGroup( i18n("Open directory"), this );
	vbl1->addWidget( bg2 );
	
	QVBoxLayout *vblg2 = new QVBoxLayout( bg2, 10 );
	vblg2->addSpacing( 10 );
	
	r4 = new QRadioButton( i18n("Favorite directory"), bg2 );
	r4->setFixedSize( r4->sizeHint() );
	vblg2->addWidget( r4, 0, AlignLeft );
	
	r5 = new QRadioButton( i18n("Last open directory"), bg2 );
	r5->setFixedSize( r5->sizeHint() );
	vblg2->addWidget( r5, 0, AlignLeft );
	
	r6 = new QRadioButton( i18n("Fixed:"), bg2 );
	r6->setFixedSize( r6->sizeHint() );
	QHBoxLayout *hblg2 = new QHBoxLayout();
	vblg2->addLayout( hblg2 );
	
	hblg2->addWidget( r6 );
	
	openLE = new QLineEdit( bg2 );
	openLE->setMinimumSize( openLE->sizeHint() );
	hblg2->addWidget( openLE );

	QPushButton *browse2 = new QPushButton( i18n("..."), bg2 );
	browse2->setFixedHeight( browse2->sizeHint().height() );
	browse2->setFixedWidth( BROWSE_WIDTH );
	hblg2->addWidget( browse2 );
        connect( browse2, SIGNAL(clicked()), SLOT(getOpenDir()) );


	
	// *******
	// *  *@@*
	// *******
	// *  *  *
	// *******
	QButtonGroup *bg3 = new QButtonGroup( i18n("Extract directory"), this );
	vbl2->addWidget( bg3 );
	
	QVBoxLayout *vblg3 = new QVBoxLayout( bg3, 10 );
	vblg3->addSpacing( 10 );
	
	r7 = new QRadioButton( i18n("Favorite directory"), bg3 );
	r7->setFixedSize( r7->sizeHint() );
	vblg3->addWidget( r7, 0, AlignLeft );
	
	r8 = new QRadioButton( i18n("Last extract directory"), bg3 );
	r8->setFixedSize( r8->sizeHint() );
	vblg3->addWidget( r8, 0, AlignLeft );
	
	r9 = new QRadioButton( i18n("Fixed:"), bg3 );
	r9->setFixedSize( r9->sizeHint() );
	QHBoxLayout *hblg3 = new QHBoxLayout();
	vblg3->addLayout( hblg3 );
	
	hblg3->addWidget( r9 );
	
	extractLE = new QLineEdit( bg3 );
	extractLE->setMinimumSize( extractLE->sizeHint() );
	hblg3->addWidget( extractLE );

	QPushButton *browse3 = new QPushButton( i18n("..."), bg3 );
	browse3->setFixedHeight( browse3->sizeHint().height() );
	browse3->setFixedWidth( BROWSE_WIDTH );
	hblg3->addWidget( browse3 );
        connect( browse3, SIGNAL(clicked()), SLOT(getExtractDir()) );


	
	// *******
	// *  *  *
	// *******
	// *  *@@*
	// *******
	QButtonGroup *bg4 = new QButtonGroup( i18n("Add directory"), this );
	vbl2->addWidget( bg4 );
	
	QVBoxLayout *vblg4 = new QVBoxLayout( bg4, 10 );
	vblg4->addSpacing( 10 );
	
	r10 = new QRadioButton( i18n("Favorite directory"), bg4 );
	r10->setFixedSize( r10->sizeHint() );
	vblg4->addWidget( r10, 0, AlignLeft );
	
	r11 = new QRadioButton( i18n("Last add directory"), bg4 );
	r11->setFixedSize( r11->sizeHint() );
	vblg4->addWidget( r11, 0, AlignLeft );
	
	r12 = new QRadioButton( i18n("Fixed:"), bg4 );
	r12->setFixedSize( r12->sizeHint() );
	QHBoxLayout *hblg4 = new QHBoxLayout();
	vblg4->addLayout( hblg4 );
	
	hblg4->addWidget( r12 );
	
	addLE = new QLineEdit( bg4 );
	addLE->setMinimumSize( addLE->sizeHint() );
	hblg4->addWidget( addLE );

	QPushButton *browse4 = new QPushButton( i18n("..."), bg4 );
	browse4->setFixedHeight( browse4->sizeHint().height() );
	browse4->setFixedWidth( BROWSE_WIDTH );
	hblg4->addWidget( browse4 );
        connect( browse4, SIGNAL(clicked()), SLOT(getAddDir()) );
		
	
	// Build the OK/Cancel buttons layout
	QHBoxLayout *hbl = new QHBoxLayout();
	mainLayout->addLayout( hbl );
	hbl->addStretch( 1 );
	
	QPushButton *ok = new QPushButton( i18n("OK"), this );
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

void DirDlg::getFavDir( )
{
  QString dir = KFileDialog::getExistingDirectory(favLE->text(), 0,
						  i18n("Archive directory"));
  if (!dir.isEmpty())
    favLE->setText(dir);
}

void DirDlg::getStartDir( )
{
  QString dir = KFileDialog::getExistingDirectory(startLE->text(), 0,
						  i18n("Start-up directory"));
  if (!dir.isEmpty())
    startLE->setText(dir);
}

void DirDlg::getOpenDir( )
{
  QString dir = KFileDialog::getExistingDirectory(openLE->text(), 0,
						  i18n("Default open directory"));
  if (!dir.isEmpty())
    openLE->setText(dir);
}

void DirDlg::getExtractDir( )
{
  QString dir = KFileDialog::getExistingDirectory(extractLE->text(), 0,
						  i18n("Default extract directory"));
  if (!dir.isEmpty())
    extractLE->setText(dir);
}

void DirDlg::getAddDir( )
{
  QString dir = KFileDialog::getExistingDirectory(addLE->text(), 0,
						  i18n("Default add directory"));
  if (!dir.isEmpty())
    addLE->setText(dir);
}

void DirDlg::favDirChanged( const QString& path)
{
	if( path.isEmpty() )	
	{
		r1->setEnabled( false );
		r4->setEnabled( false );
		r7->setEnabled( false );
		r10->setEnabled( false );
	}
	else
	{
		r1->setEnabled( true );
		r4->setEnabled( true );
		r7->setEnabled( true );
		r10->setEnabled( true );
	}

}

void DirDlg::saveConfig()
{
	int mode;
	
	data->setFavoriteDir( favLE->text() );
	
	mode = r1->isChecked() ? ArkData::FAVORITE_DIR :
		r2->isChecked() ? ArkData::LAST_OPEN_DIR : ArkData::FIXED_START_DIR;
	data->setStartDirCfg( startLE->text(), mode );

	mode = r4->isChecked() ? ArkData::FAVORITE_DIR :
		r5->isChecked() ? ArkData::LAST_OPEN_DIR : ArkData::FIXED_OPEN_DIR;
	data->setOpenDirCfg( openLE->text(), mode );

	mode = r7->isChecked() ? ArkData::FAVORITE_DIR :
		r8->isChecked() ? ArkData::LAST_EXTRACT_DIR : ArkData::FIXED_EXTRACT_DIR;
	data->setExtractDirCfg( extractLE->text(), mode );

	mode = r10->isChecked() ? ArkData::FAVORITE_DIR :
		r11->isChecked() ? ArkData::LAST_ADD_DIR : ArkData::FIXED_ADD_DIR;
	data->setAddDirCfg( addLE->text(), mode );
		
	// close the dialog now
	accept();
}

void DirDlg::initConfig()
{
	favLE->setText( data->getFavoriteDir() );
	startLE->setText( data->getFixedStartDir() );	
	openLE->setText( data->getFixedOpenDir() );	
	extractLE->setText( data->getFixedExtractDir() );	
	addLE->setText( data->getFixedAddDir() );	

	switch( data->getStartDirMode() ){
		case ArkData::FAVORITE_DIR : r1->setChecked( true ); break;	
		case ArkData::LAST_OPEN_DIR : r2->setChecked( true ); break;	
		case ArkData::FIXED_START_DIR : r3->setChecked( true ); break;	
	}

	switch( data->getOpenDirMode() ){
		case ArkData::FAVORITE_DIR : r4->setChecked( true ); break;	
		case ArkData::LAST_OPEN_DIR : r5->setChecked( true ); break;	
		case ArkData::FIXED_OPEN_DIR : r6->setChecked( true ); break;	
	}

	switch( data->getExtractDirMode() ){
		case ArkData::FAVORITE_DIR : r7->setChecked( true ); break;	
		case ArkData::LAST_EXTRACT_DIR : r8->setChecked( true ); break;	
		case ArkData::FIXED_EXTRACT_DIR : r9->setChecked( true ); break;	
	}
	
	switch( data->getAddDirMode() ){
		case ArkData::FAVORITE_DIR : r10->setChecked( true ); break;	
		case ArkData::LAST_ADD_DIR : r11->setChecked( true ); break;	
		case ArkData::FIXED_ADD_DIR : r12->setChecked( true ); break;	
	}
	
}






