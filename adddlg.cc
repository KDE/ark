#include <kapp.h>
#include <klocale.h>
#include "adddlg.h"
#include "adddlg.moc"

AddOptionsDlg::AddOptionsDlg( QWidget *parent, char *name )
	: QDialog( parent, name, TRUE )
{
	gb = new QGroupBox( klocale->translate( "Add File Options" ), this );
	gb->setAlignment( AlignLeft );
	gb->setGeometry( 10, 10, 240, 120 );

	ok = new QPushButton( klocale->translate("Ok"), this );
	ok->setGeometry( 90, 140, 70, 30 );
	connect( ok, SIGNAL( clicked() ), SLOT( accept() ) );
	cancel = new QPushButton( klocale->translate("Cancel"), this );
	cancel->setGeometry( 170, 140, 70, 30 );
	connect( cancel, SIGNAL( clicked() ), SLOT( reject() ) );

	fullcb = new QCheckBox( klocale->translate("Store Full Path"), this );
	fullcb->setGeometry( 30, 30, 130, 30 );
	
	updatecb = new QCheckBox( klocale->translate("Only Add Newer Files") , this );
	updatecb->setGeometry( 30, 70, 150, 30 );
}

bool AddOptionsDlg::onlyUpdate()
{
	return updatecb->isChecked();
}

bool AddOptionsDlg::storeFullPath()
{
	return fullcb->isChecked();
}

