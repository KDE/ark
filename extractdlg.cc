#include "extractdlg.h"
#include "extractdlg.moc"

ExtractDlg::ExtractDlg( int eo, QWidget *parent, char *name )
	: QDialog( parent, name, TRUE )
{
	QButtonGroup *gb1 = new QButtonGroup( klocale->translate( "Files"), this );
	gb1->setAlignment( AlignLeft );
	gb1->setGeometry( 260, 10, 120, 250 );
	
	QGroupBox *gb2 = new QGroupBox( klocale->translate("Extract Options"), this );
	gb2->setAlignment( AlignLeft );
	gb2->setGeometry( 10, 140, 240, 120 );

	QGroupBox *gb3 = new QGroupBox( klocale->translate("Destination"), this );
	gb3->setAlignment( AlignLeft );
	gb3->setGeometry( 10, 10, 240, 120 );	

	le = new QLineEdit( this );
	le->setGeometry( 30, 40, 200, 20 );
	le->setText( getenv( "HOME" ) );
	
	QPushButton *pb1 = new QPushButton( klocale->translate("Browse..."), this );
	pb1->setGeometry( 120, 80, 100, 30 );
	connect( pb1, SIGNAL( clicked() ), SLOT( browse() ) );

	//cb1 = new QCheckBox( klocale->translate("Overwrite"), this );
	//cb1->setGeometry( 20, 160, 100, 30 );

	cb2 = new QCheckBox( klocale->translate("Preserve Permissions"), this );
	cb2->setGeometry( 20, 190, 170, 30 );

	cb3 = new QCheckBox( klocale->translate("Filenames to Lowercase"), this );
	cb3->setGeometry( 20, 220, 160, 30 );
	
	rb1 = new QRadioButton( klocale->translate("All Files"), gb1 );
	rb1->setGeometry( 10, 30, 100, 30 );
	
	rb2 = new QRadioButton( klocale->translate("Selected File"), gb1 );
	rb2->setGeometry( 10, 50, 100, 30 );

	rb3 = new QRadioButton( klocale->translate("Pattern"), gb1 );
	rb3->setGeometry( 10, 80, 100, 30 );
	rb3->setEnabled( FALSE );

	switch( eo ) {
		case All: {
			rb1->setChecked( true );
			break;
		}
		case Selected: {
			rb2->setChecked( true );
			break;
		}
		case Pattern: {
			rb3->setChecked( true );
			break;
		}
		default: {
			rb1->setChecked( true );
			break;
		}
	}

	QLineEdit *le2 = new QLineEdit( this );
	le2->setGeometry( 270, 130, 100, 20 );
	le2->setEnabled( FALSE );

	QPushButton *pb2 = new QPushButton( klocale->translate("Ok"), this );
	pb2->setGeometry( 150, 270, 70, 30 );
	connect( pb2, SIGNAL( clicked() ), SLOT( accept() ) );
	
	QPushButton *pb3 = new QPushButton( klocale->translate("Cancel"), this );
	pb3->setGeometry( 280, 270, 70, 30 );
	connect( pb3, SIGNAL( clicked() ), SLOT( reject() ) );
}

void ExtractDlg::setMask( unsigned char mask )
{
	if( ( 0x01 & mask ) == 0 )
		cb2->setEnabled( FALSE );
	if( ( 0x02 & mask ) == 0 )
		cb3->setEnabled( FALSE );
	//if( 0x04 & mask ) == 0 )
		//cb1->setEnabled( FALSE );
}

const char *ExtractDlg::getDest()
{
	return le->text();
}

bool ExtractDlg::doOverwrite()
{
	return cb1->isChecked();
}

bool ExtractDlg::doPreservePerms()
{
	return cb2->isChecked();
}

bool ExtractDlg::doLowerCase()
{
	return cb3->isChecked();
}

const char *ExtractDlg::getPattern()
{
	return le2->text();
}

int ExtractDlg::extractOp()
{
	if( rb1->isChecked() )
		return All;
	if( rb2->isChecked() )
		return Selected;
	if( rb3->isChecked() )
		return Pattern;
	return -1;
}

void ExtractDlg::browse()
{
	KfDirDialog dd( le->text(), this, "dirdialog", TRUE );
	if( dd.exec() )
		le->setText( dd.selectedDir() );
}

