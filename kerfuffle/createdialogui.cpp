/*
 * createdialogui.cpp
 *
 * Copyright (C) 2012 basysKom GmbH <info@basyskom.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include "createdialogui.h"
#include "ui_createdialogui.h"
#include "cliinterface.h"

#include <KDebug>
#include <KFileDialog>
#include <KGlobal>
#include <KMessageBox>
#include <KMimeType>


namespace Kerfuffle
{

CreateDialogUI::CreateDialogUI(QWidget *parent) : QWidget(parent)
{
    setupUi(this);

    // fill archive formats combobox
    KMimeType::Ptr type;
    QList<int> options;
    foreach(const QString & str, Kerfuffle::supportedWriteMimeTypes()) {
        type = KMimeType::mimeType(str);
        if (type) {
            archiveFormatComboBox->addItem(type->comment(), QVariant(type->name()));
        }
        options = Kerfuffle::supportedOptions(str);
        if (!options.isEmpty()) {
            foreach(const int opt, options) {
                m_mimeTypeOptions.insert(str, opt);
            }
        }
    }

    // combobox for split file size should only accept intengers
    splitSizeComboBox->lineEdit()->setValidator(new QIntValidator(1, 1048576, this));

    connect(browseButton, SIGNAL(clicked()), SLOT(browse()));
    connect(archiveFormatComboBox, SIGNAL(activated(int)), SLOT(updateArchiveExtension()));

    // enable and disable certain parts of the UI
    connect(testArchiveCheckBox, SIGNAL(toggled(bool)), SLOT(updateUi()));
    connect(passwordGroupBox, SIGNAL(toggled(bool)), SLOT(updateUi()));
    connect(splitArchiveGroupBox, SIGNAL(toggled(bool)), SLOT(updateUi()));
    connect(splitSizeComboBox, SIGNAL(currentIndexChanged(int)), SLOT(updateUi()));
    connect(archiveFormatComboBox, SIGNAL(currentIndexChanged(int)), SLOT(updateUi()));

    updateUi();
}

void CreateDialogUI::updateUi()
{
    QString mimeType = archiveFormatComboBox->itemData(archiveFormatComboBox->currentIndex()).toString();

    encryptFileNamesCheckBox->setEnabled(passwordGroupBox->isChecked()
                                         && m_mimeTypeOptions.contains(mimeType, Kerfuffle::EncryptHeaderSwitch));
    if (!encryptFileNamesCheckBox->isEnabled()) {
        encryptFileNamesCheckBox->setChecked(false);
    }

    passwordGroupBox->setEnabled(m_mimeTypeOptions.contains(mimeType, Kerfuffle::PasswordSwitch));
    if (!passwordGroupBox->isEnabled()) {
        passwordGroupBox->setChecked(false);
    }

    compressionMethodComboBox->setEnabled(m_mimeTypeOptions.contains(mimeType, Kerfuffle::CompressionLevelSwitches));
    if (!compressionMethodComboBox->isEnabled()) {
        compressionMethodComboBox->setCurrentIndex(2);

    }

    encryptionMethodComboBox->setEnabled(m_mimeTypeOptions.contains(mimeType, Kerfuffle::EncryptionMethodSwitches));

    multithreadingCheckBox->setEnabled(m_mimeTypeOptions.contains(mimeType, Kerfuffle::MultiThreadingSwitch));
    if (!multithreadingCheckBox->isEnabled()) {
        multithreadingCheckBox->setChecked(false);
    }

    if (!testArchiveCheckBox->isChecked()) {
        deleteFilesCheckBox->setChecked(false);
    }

    if (!passwordGroupBox->isChecked()) {
        encryptContentsCheckBox->setChecked(false);
        encryptFileNamesCheckBox->setChecked(false);
    }

    splitArchiveGroupBox->setEnabled(m_mimeTypeOptions.contains(mimeType, Kerfuffle::MultiPartSwitch));
    if (splitArchiveGroupBox->isChecked() && splitArchiveGroupBox->isEnabled()) {
        if (splitSizeComboBox->currentIndex() > 0) {
            splitSizeUnitComboBox->setCurrentIndex(0);
            splitSizeUnitComboBox->setEnabled(false);
        } else {
            splitSizeUnitComboBox->setEnabled(true);
        }
    }
}

CompressionOptions CreateDialogUI::options() const
{
    CompressionOptions options;
    options[QLatin1String("ArchiveFormat")] = archiveFormatComboBox->itemData(archiveFormatComboBox->currentIndex());
    options[QLatin1String("CompressionLevel")] = compressionMethodComboBox->currentIndex();
    options[QLatin1String("TestArchive")] = testArchiveCheckBox->isChecked();
    options[QLatin1String("DeleteFilesAfterTest")] = deleteFilesCheckBox->isChecked();
    options[QLatin1String("PasswordProtectedHint")] = passwordGroupBox->isChecked();
    options[QLatin1String("EncryptContents")] = encryptContentsCheckBox->isChecked();
    options[QLatin1String("EncryptHeaderEnabled")] = encryptFileNamesCheckBox->isChecked();
    options[QLatin1String("EncryptionMethod")] = encryptionMethodComboBox->currentIndex();
    options[QLatin1String("SplitArchives")] = splitArchiveGroupBox->isChecked();
    options[QLatin1String("SplitFileSize")] = splitSizeComboBox->currentIndex();
    options[QLatin1String("SplitFileSizeFreeValue")] = splitSizeComboBox->lineEdit()->text();
    options[QLatin1String("SplitFileSizeUnit")] = splitSizeUnitComboBox->currentIndex();
    options[QLatin1String("MultiThreadingEnabled")] = multithreadingCheckBox->isChecked();
    options[QLatin1String("FixFileNameEncoding")] = utf8CheckBox->isChecked();
    options[QLatin1String("LastMimeType")] = archiveFormatComboBox->itemData(archiveFormatComboBox->currentIndex());

    return options;
}

void CreateDialogUI::setOptions(const CompressionOptions& options)
{
    archiveFormatComboBox->setCurrentIndex(archiveFormatComboBox->findData(options.value(QLatin1String("ArchiveFormat"), QLatin1String("application/zip")).toString()));
    compressionMethodComboBox->setCurrentIndex(options.value(QLatin1String("CompressionLevel"), 2).toInt());
    testArchiveCheckBox->setChecked(options.value(QLatin1String("TestArchive"), true).toBool());
    deleteFilesCheckBox->setChecked(options.value(QLatin1String("DeleteFilesAfterTest"), false).toBool());
    passwordGroupBox->setChecked(options.value(QLatin1String("PasswordProtectedHint"), false).toBool());
    encryptContentsCheckBox->setChecked(options.value(QLatin1String("EncryptContents"), false).toBool());
    encryptFileNamesCheckBox->setChecked(options.value(QLatin1String("EncryptHeaderEnabled"), false).toBool());
    encryptionMethodComboBox->setCurrentIndex(options.value(QLatin1String("EncryptionMethod"), 0).toInt());
    splitArchiveGroupBox->setChecked(options.value(QLatin1String("SplitArchives"), true).toBool());
    splitSizeComboBox->setCurrentIndex(options.value(QLatin1String("SplitFileSize"), 0).toInt());
    splitSizeComboBox->lineEdit()->setText(options.value(QLatin1String("SplitFileSizeFreeValue"), QLatin1String("")).toString());
    multithreadingCheckBox->setChecked(options.value(QLatin1String("MultiThreadingEnabled"), true).toBool());
    utf8CheckBox->setChecked(options.value(QLatin1String("FixFileNameEncoding"), true).toBool());

    updateUi();
}

void CreateDialogUI::browse()
{
    // we go the long way so we can set the mime type filter correctly
    KUrl startUrl = KUrl::fromUserInput(archiveNameLineEdit->text());
    if (startUrl.isEmpty() || !startUrl.isValid())
        startUrl = KUrl("kfiledialog:///ArkNewArchive");

    QPointer<KFileDialog> dialog = new KFileDialog(startUrl, QString(), this);
    dialog->setMimeFilter(Kerfuffle::supportedWriteMimeTypes());

    if (dialog->exec() == KDialog::Accepted) {
        const KUrl saveFileUrl = dialog->selectedUrl();
        archiveNameLineEdit->setText(saveFileUrl.path());
        checkArchiveUrl();
    }
}

bool CreateDialogUI::checkArchiveUrl()
{
    KUrl archiveUrl = KUrl::fromUserInput(archiveNameLineEdit->text());
    if (!archiveUrl.isEmpty() && archiveUrl.isValid()) {
        archiveNameLineEdit->setText(archiveUrl.path());
        updateArchiveExtension(true);
        return true;
    }

    KMessageBox::sorry(this, i18n("Please choose a file or type a valid file name."));
    return false;
}

void CreateDialogUI::updateArchiveExtension(bool updateCombobox)
{
    QString archive = archiveNameLineEdit->text();
    KMimeType::Ptr archiveType = KMimeType::findByPath(archive);

    QString typeName = archiveFormatComboBox->itemData(archiveFormatComboBox->currentIndex()).toString();
    KMimeType::Ptr type = KMimeType::mimeType(typeName);

    if (archiveType && Kerfuffle::supportedWriteMimeTypes().contains(archiveType->name())) {
        if (updateCombobox) {
            archiveFormatComboBox->setCurrentIndex(archiveFormatComboBox->findData(archiveType->name()));
            return;
        } else {
            archive.remove(QLatin1String(".") + archiveType->extractKnownExtension(archive), Qt::CaseInsensitive);
        }
    }

    if (type) {
        archiveNameLineEdit->setText(archive.append(type->mainExtension()));
    }
}


KUrl CreateDialogUI::archiveUrl() const
{
    return KUrl::fromUserInput(archiveNameLineEdit->text());
}

void CreateDialogUI::setArchiveUrl(const KUrl &archiveUrl)
{
    archiveNameLineEdit->setText(archiveUrl.path());
    updateArchiveExtension(true);
}
}

#include "createdialogui.moc"
