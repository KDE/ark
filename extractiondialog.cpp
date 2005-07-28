/**
 *
 * ark -- archiver for the KDE project
 *
 * Copyright (C)
 *
 * 2005: Henrique Pinto <henrique.pinto@kdemail.net>
 * 2002: Helio Chissini de Castro <helio@conectiva.com.br>
 * 2001: Roberto Selbach Teixeira <maragato@conectiva.com.br>
 * 2001: Corel Corporation (author: Michael Jarrett, michaelj@corel.com)
 * 1999-2000: Corel Corporation (author: Emily Ezust emilye@corel.com)
 * 1999: Francois-Xavier Duranceau duranceau@kde.org
 * 1997-1999: Rob Palmbos palm9744@kettering.edu
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include "extractiondialog.h"

#include <q3vbox.h>
#include <q3hbox.h>
#include <q3buttongroup.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <qlayout.h>

#include <klocale.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kurlrequester.h>
#include <kurlcompletion.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <kcombobox.h>
#include <klineedit.h>
#include <kdebug.h>

#include "arkutils.h"
#include "settings.h"

ExtractionDialog::ExtractionDialog( QWidget *parent, const char *name,
                                    bool enableSelected,
                                    const KURL& defaultExtractionDir,
                                    const QString &archiveName )
	: KDialogBase( parent, name, true, i18n( "Extract" ), Ok | Cancel, Ok ),
	  m_selectedButton( 0 ), m_allButton( 0 ),
	  m_selectedOnly( enableSelected ), m_extractionDirectory( defaultExtractionDir )
{
	if ( !archiveName.isNull() )
	{
		setCaption( i18n( "Extract Files From %1" ).arg( archiveName ) );
	}

	Q3VBox *vbox = makeVBoxMainWidget();
	vbox->setMinimumWidth( 480 );

	Q3HBox *header = new Q3HBox( vbox );
	header->layout()->setSpacing( 10 );

	QLabel *icon = new QLabel( header );
	icon->setPixmap( KGlobal::iconLoader()->loadIcon( "ark_extract", KIcon::Desktop ) );
	icon->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Minimum );

	if ( enableSelected )
	{
		Q3VBox *whichFiles = new Q3VBox( header );
		whichFiles->layout()->setSpacing( 6 );
		new QLabel( QString( "<qt><b><font size=\"+1\">%1</font></b></qt>" )
		            .arg( i18n( "Extract:" ) ), whichFiles );
		Q3HButtonGroup *filesGroup = new Q3HButtonGroup( whichFiles );
		m_selectedButton = new QRadioButton( i18n( "Selected files only" ), filesGroup );
		m_allButton      = new QRadioButton( i18n( "All files" ), filesGroup );

		m_selectedButton->setChecked( true );
	}
	else
	{
		new QLabel( QString( "<qt><b><font size=\"+2\">%1</font></b></qt>" )
		            .arg( i18n( "Extract all files" ) ), header );
	}

	Q3HBox *destDirBox = new Q3HBox( vbox );

	new QLabel( i18n( "Destination folder: " ), destDirBox );

	KHistoryCombo *combobox = new KHistoryCombo( true, destDirBox );
	combobox->setHistoryItems( Settings::extractionHistory() );

	KURLCompletion *comp = new KURLCompletion();
	comp->setReplaceHome( true );
	comp->setCompletionMode( KGlobalSettings::CompletionAuto );
	combobox->setCompletionObject( comp );
	combobox->setMaxCount( 20 );
	combobox->setInsertionPolicy( QComboBox::AtTop );

	m_urlRequester = new KURLRequester( combobox, destDirBox );
	m_urlRequester->setMode( KFile::Directory );

	if (!defaultExtractionDir.prettyURL().isEmpty() )
	{
		m_urlRequester->setKURL( defaultExtractionDir );
	}

	m_viewFolderAfterExtraction = new QCheckBox( i18n( "Open destination folder after extraction" ), vbox );
	m_viewFolderAfterExtraction->setChecked( Settings::openDestinationFolder() );

	connect( combobox, SIGNAL( returnPressed( const QString& ) ), combobox, SLOT( addToHistory( const QString& ) ) );
	connect( combobox->lineEdit(), SIGNAL( textChanged( const QString& ) ),
	         this, SLOT( extractDirChanged( const QString & ) ) );
}

ExtractionDialog::~ExtractionDialog()
{
	Settings::setExtractionHistory( ( static_cast<KHistoryCombo*>( m_urlRequester->comboBox() ) )->historyItems() );
	Settings::setOpenDestinationFolder( m_viewFolderAfterExtraction->isChecked() );
}

void ExtractionDialog::accept()
{

	KURLCompletion uc;
	uc.setReplaceHome( true );
	KURL p( uc.replacedPath( m_urlRequester->comboBox()->currentText() ) );

	//if p isn't local KIO and friends will complain later on
	if ( p.isLocalFile() )
	{
		QFileInfo fi( p.path() );
		if ( !fi.isDir() && !fi.exists() )
		{
			QString ltext = i18n( "Create folder %1?").arg(p.path());
			int createDir =  KMessageBox::questionYesNo( this, ltext, i18n( "Missing Folder" ) , i18n("Create Folder"), i18n("Do Not Create"));
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

	m_extractionDirectory = p;
	m_selectedOnly = m_selectedButton == 0? false : m_selectedButton->isChecked();

	static_cast<KHistoryCombo*>( m_urlRequester->comboBox() )->addToHistory( p.prettyURL() );

	KDialogBase::accept();
}

void ExtractionDialog::extractDirChanged(const QString &text )
{
	enableButtonOK(!text.isEmpty());
}

#include "extractiondialog.moc"

