/* -*- c++ -*-

$Id$

ark -- archiver for the KDE project

Copyright (C)

2002: Helio Chissini de Castro <helio@conectiva.com.br>
2001: Roberto Selbach Teixeira <maragato@conectiva.com.br>
2001: Corel Corporation (author: Michael Jarrett, michaelj@corel.com)
1999-2000: Corel Corporation (author: Emily Ezust emilye@corel.com)
1999: Francois-Xavier Duranceau duranceau@kde.org
1997-1999: Rob Palmbos palm9744@kettering.edu

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

// QT include
#include <qbuttongroup.h>
#include <qlabel.h>
#include <qapplication.h>
#include <qlayout.h>

// KDE includes
#include <kdebug.h>
#include <kfiledialog.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kcombobox.h>
#include <kurlcompletion.h>
#include <kurlrequester.h>

// application includes
#include "arkutils.h"
#include "arksettings.h"
#include "generalOptDlg.h"
#include "extractdlg.h"
#define FIRST_PAGE_WIDTH  390

ExtractDlg::ExtractDlg( ArkSettings *_settings, QWidget *parent, const char *name, const QString &prefix )
    : KDialogBase( KDialogBase::Plain, i18n("Extract"), Ok | Cancel, Ok, parent, name ),
m_settings( _settings )
{
	QFrame *mainFrame = plainPage();

	kdDebug(1601) << "+ExtractDlg::ExtractDlg" << endl;

	QGridLayout *Form1Layout = new QGridLayout( mainFrame );
	Form1Layout->setSpacing( 6 );
	Form1Layout->setMargin( 11 );

	QVBoxLayout *Layout10 = new QVBoxLayout;
	Layout10->setSpacing( 6 );
	Layout10->setMargin( 0 );

	QHBoxLayout *Layout3 = new QHBoxLayout;
	Layout3->setSpacing( 6 );
	Layout3->setMargin( 0 );

	QLabel *extractToLabel = new QLabel( mainFrame, "extractToLabel" );
	extractToLabel->setText( i18n( "Extract to:" ) );
	Layout3->addWidget( extractToLabel );

	m_extractDirCB = new KHistoryCombo( true, mainFrame, "m_extractDirCB" );

	KURLCompletion *comp = new KURLCompletion();
	comp->setReplaceHome( true );
	comp->setCompletionMode( KGlobalSettings::CompletionAuto );
	m_extractDirCB->setCompletionObject( comp );
	m_extractDirCB->setMaxCount( 20 );
	m_extractDirCB->setInsertionPolicy( QComboBox::AtTop );

	KConfig *config = m_settings->getKConfig();
	QStringList list;

	config->setGroup( "History" );
	list = config->readPathListEntry( "ExtractTo History" );
	m_extractDirCB->setHistoryItems( list );

	m_extractDirCB->setEditURL( KURL( m_settings->getExtractDir() + prefix ) );

	m_urlRequester = new KURLRequester( m_extractDirCB, mainFrame );
	m_urlRequester->setMode( KFile::Directory );

	m_extractDirCB->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );

	Layout3->addWidget( m_urlRequester );


	// Connect to the return pressed signal - optional
	connect( m_extractDirCB, SIGNAL( returnPressed( const QString& ) ), comp, SLOT( addItem( const QString& ) ) );
	connect( m_extractDirCB->lineEdit(),SIGNAL(textChanged ( const QString & )),this,SLOT(extractDirChanged(const QString & )));

	Layout10->addLayout( Layout3 );

	QButtonGroup *bg = new QButtonGroup( mainFrame, "bg" );
	bg->setTitle( i18n( "Files to Be Extracted" ) );
	bg->setColumnLayout( 0, Qt::Vertical );
	bg->layout()->setSpacing( 0 );
	bg->layout()->setMargin( 0 );
	QGridLayout *bgLayout = new QGridLayout( bg->layout() );
	bgLayout->setAlignment( Qt::AlignTop );
	bgLayout->setSpacing( 6 );
	bgLayout->setMargin( 11 );

	QVBoxLayout *Layout2 = new QVBoxLayout;
	Layout2->setSpacing( 6 );
	Layout2->setMargin( 0 );

	m_radioCurrent = new QRadioButton( bg, "m_radioCurrent" );
	m_radioCurrent->setText( i18n( "Current" ) );
	Layout2->addWidget( m_radioCurrent );

	m_radioAll = new QRadioButton( bg, "m_radioAll" );
	m_radioAll->setText( i18n( "All" ) );
	Layout2->addWidget( m_radioAll );

	m_radioSelected = new QRadioButton( bg, "m_radioSelected" );
	m_radioSelected->setText( i18n( "Selected files" ) );
	Layout2->addWidget( m_radioSelected );

	QHBoxLayout *Layout1 = new QHBoxLayout;
	Layout1->setSpacing( 6 );
	Layout1->setMargin( 0 );

	m_radioPattern = new QRadioButton( bg, "m_radioPattern" );
	m_radioPattern->setText( i18n( "Pattern:" ) );
	Layout1->addWidget( m_radioPattern );

	m_patternLE = new QLineEdit( bg, "m_patternLE" );
	Layout1->addWidget( m_patternLE );
	Layout2->addLayout( Layout1 );

	bgLayout->addLayout( Layout2, 0, 0 );
	Layout10->addWidget( bg );

	QHBoxLayout *Layout9 = new QHBoxLayout;
	Layout9->setSpacing( 6 );
	Layout9->setMargin( 0 );

	QPushButton *prefButton = new QPushButton( mainFrame, "prefButton" );
	prefButton->setText( i18n( "&Preferences..." ) );
	Layout9->addWidget( prefButton );
	QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
	Layout9->addItem( spacer );
	Layout10->addLayout( Layout9 );

	Form1Layout->addLayout( Layout10, 0, 0 );

	mainFrame->setMinimumSize(410,250);

	connect(m_patternLE, SIGNAL(textChanged(const QString &)), this, SLOT(choosePattern()));
	connect(m_patternLE, SIGNAL(returnPressed()), this, SLOT(accept()));
	connect(prefButton, SIGNAL(clicked()), this, SLOT(openPrefs()));

	//connect(browseButton, SIGNAL(clicked()), this, SLOT(browse()));
	m_radioCurrent->setChecked(true);
	enableButtonOK(!m_extractDirCB->lineEdit()->text().isEmpty());
	setFocus();
	kdDebug(1601) << "-ExtractDlg::~ExtractDlg" << endl;
}

ExtractDlg::~ExtractDlg()
{
	KConfig *config = m_settings->getKConfig();
	QStringList list;
	config->setGroup( "History" );
	list = m_extractDirCB->historyItems();
	config->writePathEntry( "ExtractTo History", list );
        config->setGroup( "ark" );
        config->writePathEntry( "lastExtractDir", m_extractDirCB->lineEdit()->text() );
}

void
ExtractDlg::extractDirChanged(const QString &text )
{
	enableButtonOK(!text.isEmpty());
}

void
ExtractDlg::disableSelectedFilesOption()
{
	m_radioSelected->setEnabled(false);
	m_radioAll->setChecked(true);
}

void
ExtractDlg::accept()
{
	kdDebug( 1601 ) << "+ExtractDlg::accept" << endl;

	KURLCompletion uc;
	uc.setReplaceHome( true );
	KURL p( uc.replacedPath(  m_extractDirCB->currentText() ) );

	//if p isn't local KIO and friends will complain later on
	if ( p.isLocalFile() )
	{
		QFileInfo fi( p.path() );
		if ( !fi.isDir() && !fi.exists() )
		{
			QString ltext = i18n( "Create folder %1?").arg(p.path());
			int createDir =  KMessageBox::questionYesNo( this, ltext, i18n( "Missing folder." ) );
			if( createDir == 4 )
			{
				return;
			}
			// create directory using filename, make sure it has trailing slash
			p.adjustPath(1);
			if( !KStandardDirs::makeDir( p.path() ) )
			{
				KMessageBox::error( this, i18n( "The folder could not be created. Please check permissions." ) );
				return;
			}
		}
		if ( !ArkUtils::haveDirPermissions( p.path() ) )
		{
			KMessageBox::error( this, i18n( "You do not have write permission to this folder. Please provide another folder." ) );
			return;
		}
	}

	m_extractDir = p;
	// you need to change the settings to change the fixed dir.
	m_settings->setLastExtractDir( p.prettyURL() );

	if ( m_radioPattern->isChecked() )
	{
		if ( m_patternLE->text().isEmpty() )
		{
			// pattern selected but no pattern? Ask user to select a pattern.
			KMessageBox::error( this, i18n( "Please provide a pattern" ) );
			return;
		}
		else
		{
			emit pattern( m_patternLE->text() );
		}
	}

	// I made it! so nothing's wrong.
	KDialogBase::accept();
	kdDebug( 1601 ) << "-ExtractDlg::accept" << endl;
}

int
ExtractDlg::extractOp()
{
	// which kind of extraction shall we do?

	if ( m_radioCurrent->isChecked() )
	{
		return ExtractDlg::Current;
	}
	if ( m_radioAll->isChecked())
	{
		return ExtractDlg::All;
	}
	if ( m_radioSelected->isChecked() )
	{
		return ExtractDlg::Selected;
	}
	if ( m_radioPattern->isChecked() )
	{
		return ExtractDlg::Pattern;
	}
	return -1;
}

KURL
ExtractDlg::extractDir()
{
	return m_extractDir;
}

void
ExtractDlg::openPrefs()
{
	GeneralOptDlg dd( m_settings, this );
	dd.exec();
}

/******************************************************************
 *           implementation of ExtractFailureDlg                  *
 ******************************************************************/
ExtractFailureDlg::ExtractFailureDlg( QStringList *list, QWidget *parent, char *name )
    : QDialog( parent, name, true, 0 )
{
	int labelHeight;
	int labelWidth;
	int boxHeight = 75;
	int boxWidth;
	int buttonHeight = 30;

	setCaption( i18n( "Failure to Extract" ) );
	QLabel *pLabel = new QLabel( this );
	pLabel->setText( i18n( "Some files already exist in your destination folder.\n"
				"The following files will not be extracted if you continue: " ) );
	labelWidth = pLabel->sizeHint().width();
	labelHeight = pLabel->sizeHint().height();

	pLabel->setGeometry( 10, 10, labelWidth, labelHeight );
	boxWidth = labelWidth;

	QListBox *pBox = new QListBox( this );
	pBox->setGeometry( 10, 10 + labelHeight + 10, boxWidth, boxHeight );
	pBox->insertStringList( *list );

	QPushButton *pOKButton = new KPushButton( KStdGuiItem::cont(), this, "OKButton" );
	pOKButton->setGeometry( labelWidth / 2 - 50, boxHeight + labelHeight + 30, 70, buttonHeight );
	connect( pOKButton, SIGNAL( pressed() ), this, SLOT( accept() ) );

	QPushButton *pCancelButton = new KPushButton( KStdGuiItem::cancel(), this, "CancelButton" );
	pCancelButton->setGeometry( labelWidth / 2 + 20, boxHeight + labelHeight + 30, 70, buttonHeight );
	connect( pCancelButton, SIGNAL( pressed() ), this, SLOT( reject() ) );
	setFixedSize( 20+labelWidth, 40+labelHeight+boxHeight+buttonHeight );
	QApplication::restoreOverrideCursor();
}

#include "extractdlg.moc"

