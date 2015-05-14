/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES ( INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION ) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * ( INCLUDING NEGLIGENCE OR OTHERWISE ) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "extractiondialog.h"
#include "settings.h"

#include <KLocalizedString>
#include <KIconLoader>
#include <KMessageBox>

#include <QDir>

#include "ui_extractiondialog.h"

namespace Kerfuffle
{

class ExtractionDialogUI: public QFrame, public Ui::ExtractionDialog
{
public:
    ExtractionDialogUI(QWidget *parent = 0)
            : QFrame(parent) {
        setupUi(this);
    }
};

ExtractionDialog::ExtractionDialog(QWidget *parent)
        : KDirSelectDialog(QUrl::fromLocalFile(QDir::homePath()), true, parent)
{
    m_ui = new ExtractionDialogUI(this);
    setExtension(m_ui);
    showExtension(true);
    setWindowTitle(i18nc("@title:window", "Extract"));
    m_ui->iconLabel->setPixmap(DesktopIcon(QLatin1String("archive-extract")));

    m_ui->filesToExtractGroupBox->hide();
    m_ui->allFilesButton->setChecked(true);
    m_ui->extractAllLabel->show();

    setSingleFolderArchive(false);

    m_ui->autoSubfolders->hide();

    loadSettings();

    connect(this, &ExtractionDialog::finished, this, &ExtractionDialog::writeSettings);
}

void ExtractionDialog::loadSettings()
{
    setOpenDestinationFolderAfterExtraction(ArkSettings::openDestinationFolderAfterExtraction());
    setCloseAfterExtraction(ArkSettings::closeAfterExtraction());
    setPreservePaths(ArkSettings::preservePaths());
}

void ExtractionDialog::setSingleFolderArchive(bool value)
{
    m_ui->singleFolderGroup->setChecked(!value);
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
    if (extractToSubfolder()) {
        if (subfolder().contains(QLatin1String( "/" ))) {
            KMessageBox::error(this, i18n("The subfolder name may not contain the character '/'."));
            return;
        }

        const QString pathWithSubfolder = url().toDisplayString(QUrl::PreferLocalFile) + QLatin1Char( '/' ) + subfolder();

        while (1) {
            if (QDir(pathWithSubfolder).exists()) {
                if (QFileInfo(pathWithSubfolder).isDir()) {
                    int overwrite = KMessageBox::questionYesNoCancel(this, xi18nc("@info", "The folder <filename>%1</filename> already exists. Are you sure you want to extract here?", pathWithSubfolder), i18n("Folder exists"), KGuiItem(i18n("Extract here")), KGuiItem(i18n("Retry")), KGuiItem(i18n("Cancel")));

                    if (overwrite == KMessageBox::No) {
                        // The user clicked Retry.
                        continue;
                    } else if (overwrite == KMessageBox::Cancel) {
                        return;
                    }
                } else {
                    KMessageBox::detailedError(this,
                                               xi18nc("@info", "The folder <filename>%1</filename> could not be created.", subfolder()),
                                               xi18nc("@info", "<filename>%1</filename> already exists, but is not a folder.", subfolder()));
                    return;
                }
            } else if (!QDir().mkdir(pathWithSubfolder)) {
                KMessageBox::detailedError(this,
                                           xi18nc("@info", "The folder <filename>%1</filename> could not be created.", subfolder()),
                                           i18n("Please check your permissions to create it."));
                return;
            }
            break;
        }
    }

    KDirSelectDialog::accept();
}

void ExtractionDialog::setSubfolder(const QString& subfolder)
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
        m_ui->selectedFilesButton->setChecked(true);
        m_ui->extractAllLabel->hide();
    } else  {
        m_ui->filesToExtractGroupBox->hide();
        m_ui->selectedFilesButton->setChecked(false);
        m_ui->extractAllLabel->show();
    }
}

bool ExtractionDialog::extractAllFiles() const
{
    return m_ui->allFilesButton->isChecked();
}

void ExtractionDialog::setAutoSubfolder(bool value)
{
    m_ui->autoSubfolders->setChecked(value);
}

bool ExtractionDialog::autoSubfolders() const
{
    return m_ui->autoSubfolders->isChecked();
}

bool ExtractionDialog::extractToSubfolder() const
{
    return m_ui->singleFolderGroup->isChecked();
}

void ExtractionDialog::setOpenDestinationFolderAfterExtraction(bool value)
{
    m_ui->openFolderCheckBox->setChecked(value);
}

void ExtractionDialog::setCloseAfterExtraction(bool value)
{
  m_ui->closeAfterExtraction->setChecked(value);
}

void ExtractionDialog::setPreservePaths(bool value)
{
    m_ui->preservePaths->setChecked(value);
}

bool ExtractionDialog::preservePaths() const
{
    return m_ui->preservePaths->isChecked();
}

bool ExtractionDialog::openDestinationAfterExtraction() const
{
    return m_ui->openFolderCheckBox->isChecked();
}

bool ExtractionDialog::closeAfterExtraction() const
{
    return m_ui->closeAfterExtraction->isChecked();
}

QUrl ExtractionDialog::destinationDirectory() const
{
    if (extractToSubfolder()) {
        //qDebug() << "Adding subfolder" << subfolder() << "to" << url();

        QUrl subUrl = url();
        subUrl.setPath(subUrl.path() + QLatin1Char('/') + subfolder());

        return subUrl;
    } else {
        return url();
    }
}

void ExtractionDialog::writeSettings()
{
    ArkSettings::setOpenDestinationFolderAfterExtraction(openDestinationAfterExtraction());
    ArkSettings::setCloseAfterExtraction(closeAfterExtraction());
    ArkSettings::setPreservePaths(preservePaths());
    ArkSettings::self()->save();
}

}


