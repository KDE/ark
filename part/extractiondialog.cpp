/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#include "extractiondialog.h"
#include "settings.h"

#include <KLocale>
#include <KIconLoader>

#include <QDir>

ExtractionDialogUI::ExtractionDialogUI( QWidget *parent )
	: QFrame( parent )
{
	setupUi( this );
}

ExtractionDialog::ExtractionDialog(QVariantMap& arguments, QWidget *parent )
	: KDirSelectDialog(),
	arguments(arguments)
{
	m_ui = new ExtractionDialogUI( this );
	//setMainWidget( m_ui );
	
	//m_ui->information->setText(QString("The root has %1 files.").arg(model.rowCount()));

	mainWidget()->layout()->addWidget(m_ui);
	setButtons( Ok | Cancel );
	setCaption( i18n( "Extract" ) );
	m_ui->iconLabel->setPixmap( DesktopIcon( "archive-extract" ) );
	m_ui->iconLabel->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Minimum );

	m_ui->filesToExtractGroupBox->hide();
	m_ui->allFilesButton->setChecked( true );
	m_ui->extractAllLabel->show();

	//m_ui->recentFolders->addItems( ArkSettings::recentExtractionFolders() );

	setCurrentUrl( KUrl( arguments.value("destination", QDir::currentPath()).toString()));

	connect( this, SIGNAL(accepted()),
			this, SLOT(updateArguments()));

	m_ui->preservePaths->setChecked(arguments.value("preservePaths", true).toBool());

	m_ui->openFolderCheckBox->setChecked(arguments.value("openDestinationAfterExtraction", false).toBool());

	m_ui->subfolder->setText(arguments.value("subfolder").toString());

	if (arguments.value("showSelectedFiles", false).toBool())
	{
		m_ui->filesToExtractGroupBox->show();
		m_ui->selectedFilesButton->setChecked( true );
		m_ui->extractAllLabel->hide();
	}

}

void ExtractionDialog::updateArguments()
{
	arguments["subfolder"] = m_ui->subfolder->text();
	arguments["destination"] = url().path();
	arguments["preservePaths"] = m_ui->preservePaths->isChecked();
	arguments["openDestinationAfterExtraction"] = m_ui->openFolderCheckBox->isChecked();

	if (m_ui->allFilesButton->isChecked())
		arguments["extract"] = "allFiles";
	else
		arguments["extract"] = "selectedFiles";

}

ExtractionDialog::~ExtractionDialog()
{
	delete m_ui;
	m_ui = 0;
}

#include "extractiondialog.moc"
