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
    options[QLatin1String("ArchiveFormat")] = archiveFormatComboBox->itemData(archiveFormatComboBox->currentIndex());
    options[QLatin1String("CompressionLevel")] = compressionMethodComboBox->currentIndex();
    options[QLatin1String("TestArchive")] = testArchiveCheckBox->isChecked();
    options[QLatin1String("DeleteFilesAfterTest")] = deleteFilesCheckBox->isChecked();
    options[QLatin1String("SetPassword")] = passwordGroupBox->isChecked();
    options[QLatin1String("EncryptContents")] = encryptContentsCheckBox->isChecked();
    options[QLatin1String("EncryptFileNames")] = encryptFileNamesCheckBox->isChecked();
    options[QLatin1String("EncryptionMethod")] = encryptionMethodComboBox->currentIndex();
    options[QLatin1String("SplitArchives")] = splitArchiveGroupBox->isChecked();
    options[QLatin1String("SplitFileSize")] = splitSizeComboBox->currentIndex();
    options[QLatin1String("SplitFileSizeFreeValue")] = splitSizeComboBox->lineEdit()->text();
    options[QLatin1String("SplitFileSizeUnit")] = splitSizeUnitComboBox->currentIndex();
    options[QLatin1String("ArchiveConflicts")] = archiveConflictsComboBox->currentIndex();
    options[QLatin1String("FileConflicts")] = fileConflictsComboBox->currentIndex();
    options[QLatin1String("UseMultithreading")] = multithreadingCheckBox->isChecked();
    options[QLatin1String("ConvertToUTF8")] = utf8CheckBox->isChecked();
    options[QLatin1String("LastMimeType")] = archiveFormatComboBox->itemData(archiveFormatComboBox->currentIndex());

    return options;
}

void CreateDialogUI::setOptions(const CompressionOptions& options)
{
    archiveFormatComboBox->setCurrentIndex(archiveFormatComboBox->findData(options.value(QLatin1String("ArchiveFormat"), QLatin1String("application/zip")).toString()));
    compressionMethodComboBox->setCurrentIndex(options.value(QLatin1String("CompressionLevel"), 2).toInt());
    testArchiveCheckBox->setChecked(options.value(QLatin1String("TestArchive"), true).toBool());
    deleteFilesCheckBox->setChecked(options.value(QLatin1String("DeleteFilesAfterTest"), false).toBool());
    passwordGroupBox->setChecked(options.value(QLatin1String("SetPassword"), true).toBool());
    encryptContentsCheckBox->setChecked(options.value(QLatin1String("EncryptContents"), false).toBool());
    encryptFileNamesCheckBox->setChecked(options.value(QLatin1String("EncryptFileNames"), false).toBool());
    encryptionMethodComboBox->setCurrentIndex(options.value(QLatin1String("EncryptionMethod"), 0).toInt());
    splitArchiveGroupBox->setChecked(options.value(QLatin1String("SplitArchives"), true).toBool());
    splitSizeComboBox->setCurrentIndex(options.value(QLatin1String("SplitFileSize"), 0).toInt());
    splitSizeComboBox->lineEdit()->setText(options.value(QLatin1String("SplitFileSizeFreeValue"), QLatin1String("")).toString());
    archiveConflictsComboBox->setCurrentIndex(options.value(QLatin1String("ArchiveConflicts"), 0).toInt());
    fileConflictsComboBox->setCurrentIndex(options.value(QLatin1String("FileConflicts"), 0).toInt());
    multithreadingCheckBox->setChecked(options.value(QLatin1String("UseMultithreading"), true).toBool());
    utf8CheckBox->setChecked(options.value(QLatin1String("ConvertToUTF8"), true).toBool());
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
