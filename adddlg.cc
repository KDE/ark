/* (c)1997 Robert Palmbos
See main.cc for license details */
#include <kapp.h>
#include <klocale.h>
#include "adddlg.h"
#include "adddlg.moc"

AddOptionsDlg::AddOptionsDlg( QWidget *parent, char *name )
	: QDialog( parent, name, TRUE )
{
	gb = new QGroupBox( klocale->translate( "Add File Options" ), this );
	gb->setAlignment( AlignLeft );

	ok = new QPushButton( klocale->translate("OK"), this );
	ok->adjustSize();
	connect( ok, SIGNAL( clicked() ), SLOT( accept() ) );
	cancel = new QPushButton( klocale->translate("Cancel"), this );
	cancel->adjustSize();
	connect( cancel, SIGNAL( clicked() ), SLOT( reject() ) );

	fullcb = new QCheckBox( klocale->translate("Store Full Path"), gb );
	fullcb->adjustSize();
	
	updatecb = new QCheckBox( klocale->translate("Only Add Newer Files") , gb );
	updatecb->adjustSize();

        setMinimumSize(70+updatecb->width(), 10+20+fullcb->height()+20+updatecb->height()+10+ok->height()+50);
}

void AddOptionsDlg::resizeEvent(QResizeEvent *e)
{
        QDialog::resizeEvent(e);
        int h_txt = fullcb->height(); // taken for the general qcheckbox height
        int w = rect().width();
        int h = rect().height();
        gb->setGeometry(10,10,w-20,h-20-40); // 40 pixels for the bottom buttons
        ok->move(20,h-10-ok->height());
        cancel->move(w-20-cancel->width(),h-10-cancel->height());
        // Now move the options inside the qcheckbox
        fullcb->move(10+20,10+20);
        updatecb->move(10+20,10+20+h_txt+20);
}

bool AddOptionsDlg::onlyUpdate()
{
	return updatecb->isChecked();
}

bool AddOptionsDlg::storeFullPath()
{
	return fullcb->isChecked();
}
