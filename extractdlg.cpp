/* -*- c++ -*-

$Id$

ark -- archiver for the KDE project

Copyright (C)

1997-1999: Rob Palmbos palm9744@kettering.edu
1999: Francois-Xavier Duranceau duranceau@kde.org
1999-2000: Corel Corporation (author: Emily Ezust emilye@corel.com)
2001: Corel Corporation (author: Michael Jarrett, michaelj@corel.com)
2001: Roberto Selbach Teixeira <maragato@conectiva.com>

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

// QT include
#include <qbuttongroup.h>
#include <qlabel.h>
#include <qapplication.h>
#include <qlayout.h>
#include <qfileinfo.h>

// KDE includes
#include <kdebug.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kcombobox.h>
#include <kurlcompletion.h>
#include <kurifilter.h>

// application includes
#include "arksettings.h"
#include "generalOptDlg.h"
#include "extractdlg.h"
#include <qpushbutton.h>

#define FIRST_PAGE_WIDTH  390

ExtractDlg::ExtractDlg( ArkSettings *_settings )
    : KDialogBase( KDialogBase::Plain, i18n("Extract"), Ok | Cancel, Ok ),
m_settings( _settings )
{
    QFrame *mainFrame = plainPage();

    kdDebug() << "+ExtractDlg::ExtractDlg" << endl;

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
    m_extractDirCB->setSizePolicy( QSizePolicy( ( QSizePolicy::SizeType ) 3, ( QSizePolicy::SizeType ) 0,
                                                m_extractDirCB->sizePolicy().hasHeightForWidth() ) );
    Layout3->addWidget( m_extractDirCB );

    KURLCompletion *comp = new KURLCompletion();
    comp->setReplaceHome( true );
    comp->setCompletionMode( KGlobalSettings::CompletionAuto );
    m_extractDirCB->setCompletionObject( comp );
    m_extractDirCB->setMaxCount( 20 );
    m_extractDirCB->setInsertionPolicy( QComboBox::AtTop );

    KConfig *config = m_settings->getKConfig();
    QStringList list;

    config->setGroup( "History" );
    list = config->readListEntry( "ExtractTo History" );
    m_extractDirCB->setHistoryItems( list );

    m_extractDirCB->setEditURL( KURL( m_settings->getExtractDir() ) );

    // Connect to the return pressed signal - optional
    connect( m_extractDirCB, SIGNAL( returnPressed( const QString& ) ), comp, SLOT( addItem( const QString& ) ) );
    connect( m_extractDirCB->lineEdit(),SIGNAL(textChanged ( const QString & )),this,SLOT(extractDirChanged(const QString & )));

    QPushButton *browseButton = new QPushButton( mainFrame, "browseButton" );
    browseButton->setText( i18n( "Browse..." ) );
    Layout3->addWidget( browseButton );
    Layout10->addLayout( Layout3 );

    QButtonGroup *bg = new QButtonGroup( mainFrame, "bg" );
    bg->setTitle( i18n( "Files to be extracted" ) );
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
    m_radioSelected->setText( i18n( "Selected Files" ) );
    Layout2->addWidget( m_radioSelected );

    QHBoxLayout *Layout1 = new QHBoxLayout;
    Layout1->setSpacing( 6 );
    Layout1->setMargin( 0 );

    m_radioPattern = new QRadioButton( bg, "m_radioPattern" );
    m_radioPattern->setText( i18n( "Pattern" ) );
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
    prefButton->setText( i18n( "&Preferences" ) );
    Layout9->addWidget( prefButton );
    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    Layout9->addItem( spacer );
    Layout10->addLayout( Layout9 );

    Form1Layout->addLayout( Layout10, 0, 0 );

    mainFrame->setMinimumSize(410,250);

    connect(m_patternLE, SIGNAL(textChanged(const QString &)),
            this, SLOT(choosePattern()));
    connect(m_patternLE, SIGNAL(returnPressed()), this, SLOT(accept()));
    connect(prefButton, SIGNAL(clicked()), this, SLOT(openPrefs()));

    connect(browseButton, SIGNAL(clicked()), this, SLOT(browse()));
    m_radioCurrent->setChecked(true);
    enableButtonOK(!m_extractDirCB->lineEdit()->text().isEmpty());
    setFocus();
    kdDebug() << "-ExtractDlg::~ExtractDlg" << endl;
}

ExtractDlg::~ExtractDlg()
{
    KConfig *config = m_settings->getKConfig();
    QStringList list;
    config->setGroup( "History" );
    list = m_extractDirCB->historyItems();
    config->writeEntry( "ExtractTo History", list );
}

void ExtractDlg::extractDirChanged(const QString &text )
{
  enableButtonOK(!text.isEmpty());
}

void ExtractDlg::disableSelectedFilesOption()
{
    m_radioSelected->setEnabled(false);
    m_radioAll->setChecked(true);
}

void ExtractDlg::accept() {

    kdDebug( 1601 ) << "+ExtractDlg::accept" << endl;
    
    KURLCompletion uc;
    KURL p( uc.replacedPath(  m_extractDirCB->currentText() ) );
 
    //if p isn't local KIO and friends will complain later on
    if ( p.isLocalFile() ){             
        QFileInfo fi( p.path() );
        if ( !fi.isDir() ) {
            KMessageBox::error( this, i18n( "Please provide a valid directory" ) );
            return;        
         }

        if ( !fi.isWritable() ) {
            KMessageBox::error( this, i18n( "You do not have write permission to this directory! Please provide another directory." ) );
            return;
        }
    }
    // you need to change the settings to change the fixed dir.
    m_settings->setLastExtractDir( p.prettyURL() ); 

    if ( m_radioPattern->isChecked() ) {
        if ( m_patternLE->text().isEmpty() ) {
            // pattern selected but no pattern? Ask user to select a pattern.
            KMessageBox::error( this,
				i18n( "Please provide a pattern" ) );
            return;
        } else {
            emit pattern( m_patternLE->text() );
        }
    }
    
    // I made it! so nothing's wrong.
    KDialogBase::accept();
    kdDebug( 1601 ) << "-ExtractDlg::accept" << endl;
}


void ExtractDlg::browse() // slot
{
   KFileDialog extractDirDlg( m_settings->getExtractDir(), QString::null, this, "extractdirdlg", true );
   extractDirDlg.setMode( KFile::Mode( KFile::Directory ) );
   extractDirDlg.setCaption(i18n("Select an Extract Directory"));
   extractDirDlg.exec();

   KURL u( extractDirDlg.selectedURL() );
   QString dirName = u.prettyURL(1);

    if (! dirName.isEmpty() )
    {
        m_extractDirCB->insertItem( dirName, 0 );
        m_extractDirCB->setCurrentItem( 0 );
    }
}

int ExtractDlg::extractOp()
{
    // which kind of extraction shall we do?

    if ( m_radioCurrent->isChecked() )
        return ExtractDlg::Current;
    if ( m_radioAll->isChecked())
        return ExtractDlg::All;
    if ( m_radioSelected->isChecked() )
        return ExtractDlg::Selected;
    if ( m_radioPattern->isChecked() )
        return ExtractDlg::Pattern;
    return -1;
}

void ExtractDlg::openPrefs()
{
    GeneralOptDlg dd( m_settings, this );
    dd.exec();
}

/******************************************************************
 *           implementation of ExtractFailureDlg                  *
 ******************************************************************/
ExtractFailureDlg::ExtractFailureDlg( QStringList *list,
                                      QWidget *parent, char *name )
    : QDialog( parent, name, true, 0 )
{
    int labelHeight, labelWidth, boxHeight = 75, boxWidth, buttonHeight = 30;
    setCaption( i18n( "Failure to Extract" ) );
    QLabel *pLabel = new QLabel( this );
    pLabel->setText( i18n( "Some files already exist in your destination directory.\nThe following files will not be extracted if you continue: " ) );
    labelWidth = pLabel->sizeHint().width();
    labelHeight = pLabel->sizeHint().height();

    pLabel->setGeometry( 10, 10, labelWidth, labelHeight );
    boxWidth = labelWidth;

    QListBox *pBox = new QListBox( this );
    pBox->setGeometry( 10, 10 + labelHeight + 10,
                       boxWidth, boxHeight );
    pBox->insertStringList( *list );

    QPushButton *pOKButton = new QPushButton( this, "OKButton" );
    pOKButton->setGeometry( labelWidth / 2 - 50, boxHeight + labelHeight + 30,
                            70, buttonHeight );
    pOKButton->setText( i18n( "Continue" ) );
    connect( pOKButton, SIGNAL( pressed() ), this, SLOT( accept() ) );

    QPushButton *pCancelButton = new QPushButton( this, "CancelButton" );
    pCancelButton->setGeometry( labelWidth / 2 + 20,
                                boxHeight + labelHeight + 30,
                                70, buttonHeight );
    pCancelButton->setText( i18n( "Cancel" ) );
    connect( pCancelButton, SIGNAL( pressed() ), this, SLOT( reject() ) );
    setFixedSize( 20+labelWidth, 40+labelHeight+boxHeight+buttonHeight );
    QApplication::restoreOverrideCursor();
}

#include "extractdlg.moc"
