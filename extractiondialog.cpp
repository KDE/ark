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

#include <qvbox.h>
#include <qhbox.h>
#include <qhbuttongroup.h>
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
#include <klineedit.h>
#include <kdebug.h>

#include "arkutils.h"

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

	QVBox *vbox = makeVBoxMainWidget();

	QHBox *header = new QHBox( vbox );
	header->layout()->setSpacing( 10 );

	QLabel *icon = new QLabel( header );
	icon->setPixmap( KGlobal::iconLoader()->loadIcon( "ark_extract", KIcon::Desktop ) );
	icon->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Minimum );

	if ( enableSelected )
	{
		QVBox *whichFiles = new QVBox( header );
		whichFiles->layout()->setSpacing( 6 );
		new QLabel( QString( "<qt><b><font size=\"+1\">%1</font></b></qt>" )
		            .arg( i18n( "Extract:" ) ), whichFiles );
		QHButtonGroup *filesGroup = new QHButtonGroup( whichFiles );
		m_selectedButton = new QRadioButton( i18n( "Selected files only" ), filesGroup );
		m_allButton      = new QRadioButton( i18n( "All files" ), filesGroup );

		m_selectedButton->setChecked( true );
	}
	else
	{
		new QLabel( QString( "<qt><b><font size=\"+2\">%1</font></b></qt>" )
		            .arg( i18n( "Extract all files" ) ), header );
	}

	QHBox *destDirBox = new QHBox( vbox );

	new QLabel( i18n( "Destination folder: " ), destDirBox );

	m_urlRequester = new KURLRequester( destDirBox );
	m_urlRequester->setMode( KFile::Directory );

	if (!defaultExtractionDir.prettyURL().isEmpty() )
	{
		m_urlRequester->setKURL( defaultExtractionDir );
	}
	else
	{
		// FIXME: Have a sensible default
		//m_urlRequester->setKURL( KGlobal::dirs()->
	}

	m_viewFolderAfterExtraction = new QCheckBox( i18n( "Open destination folder after extraction" ), vbox );
}

ExtractionDialog::~ExtractionDialog()
{
}

void ExtractionDialog::accept()
{

	KURLCompletion uc;
	uc.setReplaceHome( true );
	KURL p( uc.replacedPath( m_urlRequester->lineEdit()->text() ) );

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

	m_extractionDirectory = p;

	KDialogBase::accept();
}

void ExtractionDialog::extractDirChanged(const QString &text )
{
	enableButtonOK(!text.isEmpty());
}

#include "extractiondialog.moc"

