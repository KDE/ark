/*

 $Id $

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
#include <qcheckbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qmessagebox.h>
#include <qregexp.h>
#include <qstring.h>

// KDE includes
#include <kapp.h>
#include <kcombiview.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kdir.h>
#include <kdirlistbox.h>
#include <kfiledetaillist.h>
#include <kfilesimpleview.h>
#include <kglobal.h>
#include <klocale.h>
#include <kfileinfo.h>
#include <kfilefilter.h>
#include <ktoolbar.h>

// ark includes
#include "zipAddDlg.h"
#include "zipAddDlg.moc"


ZipAddDlg::ZipAddDlg( ZipArch *_z, ArkData *_d, QString _dir, QWidget *_parent, const char *_name )
	: KFileBaseDialog( _dir, QString::null, _parent, _name, true, false )
{
	m_zip = _z;
	m_data = _d;
	m_addClicked = false;
	
	boxLayout = 0;
	lafBox = 0;
	setMultiSelection( true );
	init();	
}

void ZipAddDlg::initGUI()
{
	kdebug(0, 1601, "+ZipAddDlg::initGUI");

	setCaption( i18n("Add...") );
	
	QVBoxLayout *mainLayout = new QVBoxLayout(this, 5);

        toolbar->setItemEnabled(1009, false);

	mainLayout->addWidget(toolbar);
        mainLayout->addWidget(fileList->widget(), 4);
        mainLayout->addSpacing(3);

	mainLayout->addSpacing( 10 );

	QHBoxLayout *hbl1 = new QHBoxLayout();
	mainLayout->addLayout( hbl1 );

	locationLabel->setText( i18n("Name: ") );
	locationLabel->setFixedSize( locationLabel->sizeHint() );
	hbl1->addWidget( locationLabel, 0, AlignTop );

	m_leNames = new QLineEdit( this );
	m_leNames->setFixedHeight( m_leNames->sizeHint().height() );
	m_leNames->setMinimumWidth( m_leNames->sizeHint().width() );
	connect( m_leNames, SIGNAL(textChanged(const QString&)), SLOT(slotSelectionChanged(const QString&)) );	
	hbl1->addWidget( m_leNames, 0, AlignTop );
	hbl1->addSpacing( 20 );
	
	QVBoxLayout *vbl1 = new QVBoxLayout();
	hbl1->addLayout( vbl1 );
	
	m_bAdd = new QPushButton( i18n("Add"), this, "add" );
	m_bAdd->setFixedSize( m_bAdd->sizeHint() );
	m_bAdd->setEnabled( false );
	connect(m_bAdd, SIGNAL(clicked()), SLOT(onAdd()));
	vbl1->addWidget( m_bAdd );
	locationLabel->setFixedHeight( m_bAdd->sizeHint().height() );
	m_leNames->setFixedHeight( m_bAdd->sizeHint().height() );

	m_bClose = new QPushButton( i18n("Cancel"), this, "cancel" );
	m_bClose->setFixedSize( m_bClose->sizeHint() );
	vbl1->addWidget( m_bClose );
        connect(m_bClose, SIGNAL(clicked()), SLOT(onClose()));

	QPushButton *bHelp = new QPushButton( i18n("Help"), this );
	bHelp->setFixedSize( bHelp->sizeHint() );
	bHelp->setEnabled( false );
	vbl1->addWidget( bHelp );
        connect(bHelp, SIGNAL(clicked()), SLOT(onHelp()));
	
	// *******
	// *@@*  *
	// *******
	
	QHBoxLayout *hbl2 = new QHBoxLayout();
	mainLayout->addLayout( hbl2 );
	
	QVBoxLayout *vblg1 = new QVBoxLayout();
	
	hbl2->addLayout( vblg1 );
	
	QLabel *l1 = new QLabel( i18n("Action:"), this );
	l1->setFixedSize( l1->sizeHint() );
	vblg1->addWidget( l1, 0, AlignLeft | AlignTop );
	
	cb1 = new QComboBox( false, this );
	cb1->insertItem( i18n("Add and update files") );
	cb1->insertItem( i18n("Freshen (changed) files") );
	cb1->insertItem( i18n("Move files (delete files)") );
	cb1->setFixedHeight( cb1->sizeHint().height() );
	cb1->setMinimumWidth( cb1->sizeHint().width() );
	vblg1->addWidget( cb1, 0, AlignLeft | AlignTop );

	vblg1->addSpacing( 10 );
	
	QLabel *l2 = new QLabel( i18n("Compression:"), this );
	l2->setFixedSize( l2->sizeHint() );
	vblg1->addWidget( l2, 0, AlignLeft );
	
	cb2 = new QComboBox( false, this );
	cb2->insertItem( i18n("Maximum ( 9 )") );
	cb2->insertItem( i18n("Good ( 7 )") );
	cb2->insertItem( i18n("Normal ( 5 )") );
	cb2->insertItem( i18n("Small ( 3 )") );
	cb2->insertItem( i18n("Minimum ( 1 )") );
	cb2->insertItem( i18n("None, store only") );
	cb2->setFixedHeight( cb2->sizeHint().height() );
	cb2->setMinimumWidth( cb1->minimumWidth() );
	vblg1->addWidget( cb2, 0, AlignLeft );
		
	vblg1->addStretch( 1 );
	
	// *******
	// *  *@@*
	// *******
	
	QButtonGroup *bg2 = new QButtonGroup( i18n("Options"), this );
	hbl2->addWidget( bg2 );
	
	QVBoxLayout *vblg2 = new QVBoxLayout( bg2, 10 );
	vblg2->addSpacing( 10 );
	
	c1 = new QCheckBox( i18n("Recurse into directories"), bg2 );
	c1->setFixedSize( c1->sizeHint() );
	vblg2->addWidget( c1, 0, AlignLeft );
	
	c2 = new QCheckBox( i18n("Junk directory names"), bg2 );
	c2->setFixedSize( c2->sizeHint() );
	vblg2->addWidget( c2, 0, AlignLeft );
	
	c3 = new QCheckBox( i18n("Force MSDOS (8+3) file names"), bg2 );
	c3->setFixedSize( c3->sizeHint() );
	vblg2->addWidget( c3, 0, AlignLeft );
	
	c4 = new QCheckBox( i18n("Convert LF to CRLF"), bg2 );
	c4->setFixedSize( c4->sizeHint() );
	vblg2->addWidget( c4, 0, AlignLeft );
	
	if ( myStatusLine )
		mainLayout->addWidget( myStatusLine, 0 );

	bOk->hide();
	bCancel->hide();
	locationEdit->hide();
		
	mainLayout->activate();

	resize( minimumSize() );

	fileList->connectDirSelected(this, SLOT(dirActivated(KFileInfo*)));
	fileList->connectFileSelected(this, SLOT(fileActivated(KFileInfo*)));
	fileList->connectFileHighlighted(this, SLOT(fileHighlighted(KFileInfo*)));
	connect(this, SIGNAL(fileHighlighted(const QString &)), SLOT(slotFileHighlighted(const QString&)));
	connect(this, SIGNAL(fileSelected(const QString &)), SLOT(slotFileSelected(const QString&)));

	kdebug(0, 1601, "-ZipAdd::initGUI");
}

bool ZipAddDlg::getShowFilter()
{
	return false;
}

KFileInfoContents *ZipAddDlg::initFileList( QWidget *parent )
{

    bool mixDirsAndFiles = true;
//	KGlobal::config()->readBoolEntry("MixDirsAndFiles",
//					 false);

    bool showDetails =
	(KGlobal::config()->readEntry("ViewStyle",
				      "DetailView") == "DetailView");

    bool useSingleClick = false;
//	KGlobal::config()->readBoolEntry("SingleClick", true);

    QDir::SortSpec sort = static_cast<QDir::SortSpec>(dir->sorting() &
                                                      QDir::SortByMask);

    if (KGlobal::config()->readBoolEntry("KeepDirsFirst",
					 true))
        sort = static_cast<QDir::SortSpec>(sort | QDir::DirsFirst);

    dir->setSorting(sort);

    if (!mixDirsAndFiles)

	return new KCombiView(KCombiView::DirList,
				  showDetails ? KCombiView::DetailView
				  : KCombiView::SimpleView,
				  useSingleClick, dir->sorting(),
				  parent, "_combi");

    else

	if (showDetails)
	    return new KFileDetailList(useSingleClick, dir->sorting(), parent, "_details");
	else
	    return new KFileSimpleView(useSingleClick, dir->sorting(), parent, "_simple");

}

void ZipAddDlg::onAdd()
{
	if( !m_addClicked ){
		m_bClose->setText( i18n("Close") );
		m_addClicked = true;
	}

	saveConfig();
	
	m_zip->add( location(), mode(), compression(),
		c1->isChecked(), c2->isChecked(), c3->isChecked(), c4->isChecked() );
}

void ZipAddDlg::onClose()
{
	saveConfig();
	m_addClicked ? accept()	: reject();
}

void ZipAddDlg::onHelp()
{
}

void ZipAddDlg::slotSelectionChanged(const QString& sel)
{
	QRegExp exp( sel, true, true );
	
	m_bAdd->setEnabled( exp.isValid() );
}

void ZipAddDlg::slotFileHighlighted(const QString& _fname)
{
	m_leNames->setText( _fname );
}

void ZipAddDlg::slotFileSelected(const QString& _fname)
{
	kdebug(0, 1601, "selected: %s", selectedFile().ascii() );
}

void ZipAddDlg::saveConfig()
{
	m_data->setZipAddRecurseDirs( c1->isChecked() );	
	m_data->setZipAddJunkDirs( c2->isChecked() );	
	m_data->setZipAddMSDOS( c3->isChecked() );	
	m_data->setZipAddConvertLF( c4->isChecked() );	
}

QString ZipAddDlg::location()
{
	if(lastDirectory->left(5) != "file:")
		kdebug(3, 1601, "Only file protocol is supported here !");
		
	return QString(lastDirectory->right(lastDirectory->length()-5) + m_leNames->text());
}

int ZipAddDlg::mode()
{
	
}

QString ZipAddDlg::compression()
{
	switch( cb2->currentItem() )
	{
		case 0 : return QString("-9"); break;
		case 1 : return QString("-7"); break;
		case 2 : return QString("-5"); break;
		case 3 : return QString("-3"); break;
		case 4 : return QString("-1"); break;
		case 5 : return QString("-0"); break;
	}
}