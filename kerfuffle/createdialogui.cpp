/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009,2011 Raphael Kubo da Costa <kubito@gmail.com>
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

#include <KDebug>
#include <KFileDialog>
#include <KGlobal>
#include <KMessageBox>
#include <KMimeType>

#include "ui_createdialogui.h"
#include "createdialogui.h"


namespace Kerfuffle
{

CreateDialogUI::CreateDialogUI(QWidget *parent) : QWidget(parent)
{
    setupUi(this);

    // fill archive formats combobox
    QString str;
    KMimeType::Ptr type;
    foreach (str, Kerfuffle::supportedWriteMimeTypes()) {
        type = KMimeType::mimeType(str);
        if( type ) {
            archiveFormatComboBox->addItem(type->comment(), QVariant(type->name()));
        }
    }

    // combobox for splite file size should only accept intengers
    splitSizeComboBox->lineEdit()->setValidator(new QIntValidator(1, 1048576, this));

    connect(browseButton, SIGNAL(clicked()), SLOT(browse()));
    connect(archiveFormatComboBox, SIGNAL(activated(int)), SLOT(updateArchiveExtension()));
}

CompressionOptions CreateDialogUI::options() const
{
    CompressionOptions options;
    options["ArchiveFormat"] = archiveFormatComboBox->itemData(archiveFormatComboBox->currentIndex());
    options["CompressionLevel"] = compressionMethodComboBox->currentIndex();
    options["TestArchive"] = testArchiveCheckBox->isChecked();
    options["DeleteFilesAfterTest"] = deleteFilesCheckBox->isChecked();
    options["SetPassword"] = passwordGroupBox->isChecked();
    options["EncryptContents"] = encryptContentsCheckBox->isChecked();
    options["EncryptFileNames"] = encryptFileNamesCheckBox->isChecked();
    options["EncryptionMethod"] = encryptionMethodComboBox->currentIndex();
    options["SplitArchives"] = splitArchiveGroupBox->isChecked();
    options["SplitFileSize"] = splitSizeComboBox->currentIndex();
    options["SplitFileSizeFreeValue"] = splitSizeComboBox->lineEdit()->text();
    options["SplitFileSizeUnit"] = splitSizeUnitComboBox->currentIndex();
    options["ArchiveConflicts"] = archiveConflictsComboBox->currentIndex();
    options["FileConflicts"] = fileConflictsComboBox->currentIndex();
    options["UseMultithreading"] = multithreadingCheckBox->isChecked();
    options["ConvertToUTF8"] = utf8CheckBox->isChecked();
    options["LastMimeType"] = archiveFormatComboBox->itemData(archiveFormatComboBox->currentIndex());

    return options;
}

void CreateDialogUI::setOptions(const CompressionOptions& options)
{
    archiveFormatComboBox->setCurrentIndex(archiveFormatComboBox->findData(options.value("ArchiveFormat", "application/zip").toString()));
    compressionMethodComboBox->setCurrentIndex(options.value("CompressionLevel", 2).toInt());
    testArchiveCheckBox->setChecked(options.value("TestArchive", true).toBool());
    deleteFilesCheckBox->setChecked(options.value("DeleteFilesAfterTest", false).toBool());
    passwordGroupBox->setChecked(options.value("SetPassword", true).toBool());
    encryptContentsCheckBox->setChecked(options.value("EncryptContents", false).toBool());
    encryptFileNamesCheckBox->setChecked(options.value("EncryptFileNames", false).toBool());
    encryptionMethodComboBox->setCurrentIndex(options.value("EncryptionMethod", 0).toInt());
    splitArchiveGroupBox->setChecked(options.value("SplitArchives", true).toBool());
    splitSizeComboBox->setCurrentIndex(options.value("SplitFileSize", 0).toInt());
    splitSizeComboBox->lineEdit()->setText(options.value("SplitFileSizeFreeValue", "").toString());
    archiveConflictsComboBox->setCurrentIndex(options.value("ArchiveConflicts", 0).toInt());
    fileConflictsComboBox->setCurrentIndex(options.value("FileConflicts", 0).toInt());
    multithreadingCheckBox->setChecked(options.value("UseMultithreading", true).toBool());
    utf8CheckBox->setChecked(options.value("ConvertToUTF8", true).toBool());
}

void CreateDialogUI::browse()
{
    // we go the long way so we can set the mime type filter correctly
    KUrl startUrl = KUrl::fromUserInput(archiveNameLineEdit->text());
    if( startUrl.isEmpty() || !startUrl.isValid() )
        startUrl = KUrl("kfiledialog:///ArkNewArchive");

    KFileDialog dialog(startUrl, QString(), this );
    dialog.setMimeFilter( Kerfuffle::supportedWriteMimeTypes() );

    if ( dialog.exec() != KDialog::Accepted) {
        return;
    }

    const KUrl saveFileUrl = dialog.selectedUrl();
    archiveNameLineEdit->setText(saveFileUrl.path());
    checkArchiveUrl();
}

bool CreateDialogUI::checkArchiveUrl()
{
    KUrl archiveUrl = KUrl::fromUserInput(archiveNameLineEdit->text());
    if (!archiveUrl.isEmpty() && archiveUrl.isValid()) {
        archiveNameLineEdit->setText(archiveUrl.path());
        updateArchiveExtension( true );
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
        }
        else {
            archive.remove(QString(".").append(archiveType->extractKnownExtension(archive)), Qt::CaseInsensitive);
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
