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

#include "iostream.h"

// Qt includes
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qstring.h>

// KDE includes
#include <kapp.h>
#include <kconfig.h>
#include <kdir.h>
#include <kdirlistbox.h>
#include <klocale.h>
#include <kfileinfo.h>
#include <kfilefilter.h>
#include <ktoolbar.h>

// ark includes
#include "ZipExtractDlg.h"
#include "ZipExtractDlg.moc"


ZipExtractDlg::ZipExtractDlg( QString dirName, QWidget *parent, const char *name )
	: KFileBaseDialog( dirName, QString::null, parent, name, true, false )
{
	boxLayout = 0;
	lafBox = 0;
	cerr << "before init()\n";
	init();	
	cerr << "after init()\n";
}

void ZipExtractDlg::initGUI()
{
    cerr << "Entered initGUI()\n";

	setCaption( i18n("Extract to...") );
	
	QVBoxLayout *mainLayout = new QVBoxLayout(this, 5);

cerr << "p1\n";

	toolbar->setFixedHeight( toolbar->height() );
	toolbar->setMinimumWidth( toolbar->width() );
	toolbar->hideItem(8);
	mainLayout->addSpacing(toolbar->height());
	mainLayout->addSpacing( 5 );

cerr << "p1\n";
cerr << toolbar->height() << "\n";
cerr << toolbar->width() << "\n";
cerr << "p1\n";
cerr << fileList->widget()->height() << "\n";
cerr << fileList->widget()->width() << "\n";
cerr << fileList->widget()->sizeHint().height() << "\n";
cerr << fileList->widget()->sizeHint().width() << "\n";
	fileList->widget()->setMinimumHeight( 200 );
	mainLayout->addWidget(fileList->widget(), 4);

	mainLayout->addSpacing( 10 );

	QHBoxLayout *hbl1 = new QHBoxLayout();
	mainLayout->addLayout( hbl1 );

	locationLabel->setText( i18n("Extract to: ") );
	locationLabel->setFixedSize( locationLabel->sizeHint() );
	hbl1->addWidget( locationLabel, 0, AlignTop );
	locationEdit->setFixedHeight( locationEdit->sizeHint().height() );
	locationEdit->setMinimumWidth( locationEdit->sizeHint().width() );
	hbl1->addWidget( locationEdit, 0, AlignTop );

	QVBoxLayout *vbl1 = new QVBoxLayout();
	hbl1->addLayout( vbl1 );
	QPushButton *bExtract = new QPushButton( i18n("Extract"), this );
	bExtract->setFixedSize( bExtract->sizeHint() );
	vbl1->addWidget( bExtract );
	locationLabel->setFixedHeight( bExtract->sizeHint().height() );
	locationEdit->setFixedHeight( bExtract->sizeHint().height() );

	QPushButton *cancel = new QPushButton( i18n("Cancel"), this );
	cancel->setFixedSize( cancel->sizeHint() );
	vbl1->addWidget( cancel );
        connect(cancel, SIGNAL(clicked()), SLOT(reject()));

	// *******
	// *@@*  *
	// *******
	
	QHBoxLayout *hbl2 = new QHBoxLayout();
	mainLayout->addLayout( hbl2 );
	
	QButtonGroup *bg1 = new QButtonGroup( i18n("Files"), this );
	hbl2->addWidget( bg1 );
	
	QVBoxLayout *vblg1 = new QVBoxLayout( bg1, 10 );
	vblg1->addSpacing( 10 );
	
	QRadioButton *r1 = new QRadioButton( i18n("Selected files"), bg1 );
	r1->setFixedSize( r1->sizeHint() );
	r1->setEnabled( false );
	vblg1->addWidget( r1, 0, AlignLeft );
	
	QRadioButton *r2 = new QRadioButton( i18n("All files"), bg1 );
	r2->setFixedSize( r2->sizeHint() );
	r2->setChecked( true );
	vblg1->addWidget( r2, 0, AlignLeft );
	
	QRadioButton *r3 = new QRadioButton( i18n("Files: "), bg1 );
	r3->setFixedSize( r3->sizeHint() );
	QHBoxLayout *hblg1 = new QHBoxLayout();
	vblg1->addLayout( hblg1 );
	
	hblg1->addWidget( r3 );
	
	QLineEdit *le1 = new QLineEdit( bg1 );
	le1->setMinimumSize( le1->sizeHint() );
	hblg1->addWidget( le1 );

	// *******
	// *  *@@*
	// *******
	
	QButtonGroup *bg2 = new QButtonGroup( i18n("Options"), this );
	hbl2->addWidget( bg2 );
	
	QVBoxLayout *vblg2 = new QVBoxLayout( bg2, 10 );
	vblg2->addSpacing( 10 );
	
	QCheckBox *r4 = new QCheckBox( i18n("Overwrite existing files"), bg2 );
	r4->setFixedSize( r4->sizeHint() );
	vblg2->addWidget( r4, 0, AlignLeft );
	
	QCheckBox *r5 = new QCheckBox( i18n("Junk paths"), bg2 );
	r5->setFixedSize( r5->sizeHint() );
	vblg2->addWidget( r5, 0, AlignLeft );
	
	QCheckBox *r6 = new QCheckBox( i18n("Restore UID/GID"), bg2 );
	r6->setFixedSize( r6->sizeHint() );
	vblg2->addWidget( r6, 0, AlignLeft );
	

cerr << "p1\n";
cerr << "p1\n";
    if ( myStatusLine )
	mainLayout->addWidget( myStatusLine, 0 );

    bOk->hide();
    bCancel->hide();
    bHelp->hide();

    mainLayout->activate();
cerr << "p1\n";
cerr << toolbar->height() << "\n";
cerr << toolbar->width() << "\n";
cerr << "p1\n";
cerr << fileList->widget()->height() << "\n";
cerr << fileList->widget()->width() << "\n";
cerr << fileList->widget()->sizeHint().height() << "\n";
cerr << fileList->widget()->sizeHint().width() << "\n";

	resize( minimumSize() );
//	setFixedWidth( width() );

cerr << "p1\n";
cerr << toolbar->height() << "\n";
cerr << toolbar->width() << "\n";
cerr << "p1\n";
cerr << fileList->widget()->height() << "\n";
cerr << fileList->widget()->width() << "\n";
cerr << fileList->widget()->sizeHint().height() << "\n";
cerr << fileList->widget()->sizeHint().width() << "\n";

    fileList->connectDirSelected(this, SLOT(dirActivated(KFileInfo*)));
    fileList->connectFileSelected(this, SLOT(fileActivated(KFileInfo*)));
    fileList->connectFileHighlighted(this, SLOT(fileHighlighted(KFileInfo*)));

    cerr << "Exited initGUI()\n";

}

bool ZipExtractDlg::getShowFilter()
{
	return false;
}

KFileInfoContents* ZipExtractDlg::initFileList( QWidget *parent )
{
	bool useSingleClick = kapp->getConfig()->readBoolEntry("SingleClick", true);
	return new KDirListBox( useSingleClick, dir->sorting(), parent, "_dirs" );
}

