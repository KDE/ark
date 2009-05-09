/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Harald Hvaal <haraldhv@stud.ntnu.no>
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

#include <KLocale>
#include <KIconLoader>
#include <KMessageBox>
#include <KStandardDirs>
#include <KDebug>

#include <QDir>

#include "ui_extractiondialog.h"

namespace Kerfuffle
{

	class ExtractionDialogUI: public QFrame, public Ui::ExtractionDialog
	{
		public:
			ExtractionDialogUI( QWidget *parent = 0 )
				: QFrame( parent )
			{
				setupUi( this );
			}
	};

	ExtractionDialog::ExtractionDialog(QWidget *parent )
		: KDirSelectDialog()
	{
		//TODO: send the parent pointer to superclass
		Q_UNUSED(parent  );

		m_ui = new ExtractionDialogUI( this );

		mainWidget()->layout()->addWidget(m_ui);
		setCaption( i18n( "Extract" ) );
		m_ui->iconLabel->setPixmap( DesktopIcon( "archive-extract" ) );
		m_ui->iconLabel->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Minimum );

		m_ui->filesToExtractGroupBox->hide();
		m_ui->allFilesButton->setChecked( true );
		m_ui->extractAllLabel->show();

		setSingleFolderArchive(false);

		m_ui->autoSubfolders->hide();

		//we want the warning text to show through even if the groupbox is disabled
		QPalette warningPalette = m_ui->singleFolder->palette();
		warningPalette.setColor(QPalette::Disabled, QPalette::Text, warningPalette.color(QPalette::Active, QPalette::Text));
		m_ui->singleFolder->setPalette(warningPalette);

	}

	void ExtractionDialog::setSingleFolderArchive(bool value)
	{
		if (value) {
			m_ui->singleFolderGroup->setChecked(false);
			m_ui->singleFolder->show();
		} else {
			m_ui->singleFolderGroup->setChecked(true);
			m_ui->singleFolder->hide();
		}
	}

	void ExtractionDialog::batchModeOption()
	{
		m_ui->autoSubfolders->show();
		m_ui->autoSubfolders->setEnabled(true);
		m_ui->singleFolderGroup->hide();
		m_ui->extractAllLabel->setText(i18n("Extract multiple archives"));
	}

	void ExtractionDialog::accept()
	{
		if (m_ui->subfolder->text().contains("/"))
		{
			KMessageBox::error(NULL, i18n("The subfolder name may not contain the character '/'."));
			return;
		}

		if (!KStandardDirs::exists(url().path(KUrl::AddTrailingSlash)))
		{

			QString ltext = i18n( "Create folder %1?", url().path());
			int createDir =  KMessageBox::questionYesNo( this, ltext, i18n( "Missing Folder" ) , KGuiItem(i18n("Create Folder")), KGuiItem(i18n("Do Not Create")));
			if( createDir == KMessageBox::No )
			{
				return;
			}

			if( !KStandardDirs::makeDir( url().path( KUrl::AddTrailingSlash) ) )
			{
				KMessageBox::error( this, i18n( "The folder could not be created. Please check permissions." ) );
				return;
			}
		}

		QString pathWithSubfolder = url().path(KUrl::AddTrailingSlash) + subfolder() + '/';
		kDebug (1601) << pathWithSubfolder << " exists: " << KStandardDirs::exists(pathWithSubfolder);
		if (extractToSubfolder() && KStandardDirs::exists(pathWithSubfolder)) {
				int overWrite = KMessageBox::questionYesNo( NULL, i18n("The folder '%1' already exists. Are you sure you want to extract here?",pathWithSubfolder ), i18n("Folder exists") , KGuiItem(i18n("Extract here")), KGuiItem(i18n("Cancel")));
				if( overWrite == KMessageBox::No )
				{
					//TODO: choosing retry should also be possible, so one does
					//not have to do the procedure one more time.
					return;
				}
		}
		

		KDirSelectDialog::accept();

	}

	void ExtractionDialog::setCurrentUrl(const QString& url)
	{
		KDirSelectDialog::setCurrentUrl(url);
	}

	void ExtractionDialog::setSubfolder(QString subfolder)
	{
		m_ui->subfolder->setText(subfolder);
	}

	QString ExtractionDialog::subfolder() const
	{
		return m_ui->subfolder->text();
	}

	ExtractionDialog::~ExtractionDialog()
	{
		delete m_ui;
		m_ui = 0;
	}

	void ExtractionDialog::setShowSelectedFiles(bool value)
	{
		if (value) {
			m_ui->filesToExtractGroupBox->show();
			m_ui->selectedFilesButton->setChecked( true );
			m_ui->extractAllLabel->hide();
		} else  {
			m_ui->filesToExtractGroupBox->hide();
			m_ui->selectedFilesButton->setChecked( false );
			m_ui->extractAllLabel->show();
		}
	}

	bool ExtractionDialog::extractAllFiles()
	{
		return m_ui->allFilesButton->isChecked();
	}

	void ExtractionDialog::setAutoSubfolder(bool value)
	{
		m_ui->autoSubfolders->setChecked(value);
	}

	bool ExtractionDialog::autoSubfolders()
	{
		return m_ui->autoSubfolders->isChecked();
	}

	bool ExtractionDialog::extractToSubfolder()
	{
		return m_ui->singleFolderGroup->isChecked();
	}

	void ExtractionDialog::setOpenDestinationFolderAfterExtraction(bool value)
	{
		m_ui->openFolderCheckBox->setChecked(value);
	}

	void ExtractionDialog::setPreservePaths(bool value)
	{
		m_ui->preservePaths->setChecked(value);
	}

	bool ExtractionDialog::preservePaths()
	{
		return m_ui->preservePaths->isChecked();
	}


	bool ExtractionDialog::openDestinationAfterExtraction()
	{
		return m_ui->openFolderCheckBox->isChecked();
	}

	KUrl ExtractionDialog::destinationDirectory()
	{
		return url().path(KUrl::AddTrailingSlash);
	}

}

#include "extractiondialog.moc"
