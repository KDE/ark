/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009,2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
 * Copyright (C) 2015 Elvis Angelaccio <elvis.angelaccio@kde.org>
 * Copyright (C) 2016 Ragnar Thomsen <rthomsen6@gmail.com>
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

#include "createdialog.h"
#include "archiveformat.h"
#include "ark_debug.h"
#include "ui_createdialog.h"
#include "mimetypes.h"

#include <KMessageBox>
#include <KSharedConfig>
#include <KUrlRequester>

#include <QDebug>
#include <QMimeDatabase>
#include <QUrl>

namespace Kerfuffle
{
class CreateDialogUI: public QWidget, public Ui::CreateDialog
{
    Q_OBJECT

public:
    CreateDialogUI(QWidget *parent = nullptr)
            : QWidget(parent) {
        setupUi(this);
    }
};

CreateDialog::CreateDialog(QWidget *parent,
                           const QString &caption,
                           const QUrl &startDir)
        : QDialog(parent, Qt::Dialog)
{
    setWindowTitle(caption);
    setModal(true);

    m_supportedMimeTypes = m_pluginManger.supportedWriteMimeTypes(PluginManager::SortByComment);

    m_vlayout = new QVBoxLayout();
    setLayout(m_vlayout);

    m_ui = new CreateDialogUI(this);
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    m_ui->destFolderUrlRequester->setMode(KFile::Directory);
    if (startDir.isEmpty()) {
        m_ui->destFolderUrlRequester->setUrl(QUrl::fromLocalFile(QDir::currentPath() + QLatin1Char('/')));
    } else {
        m_ui->destFolderUrlRequester->setUrl(startDir);
    }

    // Populate combobox with mimetypes.
    for (const QString &type : qAsConst(m_supportedMimeTypes)) {
        m_ui->mimeComboBox->addItem(QMimeDatabase().mimeTypeForName(type).comment());
    }

    connect(m_ui->filenameLineEdit, &QLineEdit::textChanged, this, &CreateDialog::slotFileNameEdited);
    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(this, &QDialog::accepted, this, &CreateDialog::slotUpdateDefaultMimeType);
    connect(m_ui->mimeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CreateDialog::slotUpdateWidgets);
    connect(m_ui->mimeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CreateDialog::slotUpdateFilenameExtension);

    m_vlayout->addWidget(m_ui);

    m_ui->optionsWidget->setMimeType(currentMimeType());

    loadConfiguration();

    layout()->setSizeConstraint(QLayout::SetFixedSize);
    m_ui->filenameLineEdit->setFocus();
    slotUpdateFilenameExtension(m_ui->mimeComboBox->currentIndex());
}

void CreateDialog::setFileName(const QString &fileName)
{
    m_ui->filenameLineEdit->setText(fileName);

    const QString detectedSuffix = QMimeDatabase().suffixForFileName(fileName);
    if (currentMimeType().suffixes().contains(detectedSuffix)) {
        m_ui->filenameLineEdit->setSelection(0, fileName.length() - detectedSuffix.length() - 1);
    } else {
        m_ui->filenameLineEdit->selectAll();
    }
}

void CreateDialog::slotFileNameEdited(const QString &fileName)
{
    const QMimeType mimeFromFileName = QMimeDatabase().mimeTypeForFile(fileName, QMimeDatabase::MatchExtension);

    if (m_supportedMimeTypes.contains(mimeFromFileName.name())) {
        setMimeType(mimeFromFileName.name());
    }

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!fileName.isEmpty());
}

void CreateDialog::slotUpdateWidgets(int index)
{
    m_ui->optionsWidget->setMimeType(QMimeDatabase().mimeTypeForName(m_supportedMimeTypes.at(index)));
}

void CreateDialog::slotUpdateFilenameExtension(int index)
{
    m_ui->chkAddExtension->setText(i18nc("the argument is a file extension (the period is not a typo)", "Automatically add .%1",
                                         QMimeDatabase().mimeTypeForName(m_supportedMimeTypes.at(index)).preferredSuffix()));
}

QUrl CreateDialog::selectedUrl() const
{
    QString fileName = m_ui->filenameLineEdit->text();
    QString dir = m_ui->destFolderUrlRequester->url().toLocalFile();
    if (dir.trimmed().endsWith(QLatin1Char('/'))) {
        dir = dir.trimmed();
    }

    if (m_ui->chkAddExtension->isChecked()) {
        QString detectedSuffix = QMimeDatabase().suffixForFileName(m_ui->filenameLineEdit->text().trimmed());

        if (!currentMimeType().suffixes().contains(detectedSuffix)) {
            if (!fileName.endsWith(QLatin1Char('.'))) {
                fileName.append(QLatin1Char('.'));
            }
            fileName.append(currentMimeType().preferredSuffix());
        }
    }

    if (!dir.endsWith(QLatin1Char('/'))) {
        dir.append(QLatin1Char('/'));
    }
    return QUrl::fromLocalFile(dir + fileName);
}

int CreateDialog::compressionLevel() const
{
    return m_ui->optionsWidget->compressionLevel();
}

QString CreateDialog::compressionMethod() const
{
    return m_ui->optionsWidget->compressionMethod();
}

QString CreateDialog::encryptionMethod() const
{
    return m_ui->optionsWidget->encryptionMethod();
}

ulong CreateDialog::volumeSize() const
{
    return m_ui->optionsWidget->volumeSize();
}

QString CreateDialog::password() const
{
    return m_ui->optionsWidget->password();
}

bool CreateDialog::isEncryptionAvailable() const
{
    return m_ui->optionsWidget->isEncryptionAvailable();
}

bool CreateDialog::isEncryptionEnabled() const
{
    return m_ui->optionsWidget->isEncryptionEnabled();
}

bool CreateDialog::isHeaderEncryptionAvailable() const
{
    return m_ui->optionsWidget->isHeaderEncryptionAvailable();
}

bool CreateDialog::isHeaderEncryptionEnabled() const
{
    return m_ui->optionsWidget->isHeaderEncryptionEnabled();
}

void CreateDialog::accept()
{
    if (!isEncryptionEnabled()) {
        QDialog::accept();
        return;
    }

    switch (m_ui->optionsWidget->passwordStatus()) {
    case KNewPasswordWidget::WeakPassword:
    case KNewPasswordWidget::StrongPassword:
        QDialog::accept();
        break;
    case KNewPasswordWidget::PasswordNotVerified:
        KMessageBox::error(nullptr, i18n("The chosen password does not match the given verification password."));
        break;
    default:
        break;
    }
}

void CreateDialog::slotUpdateDefaultMimeType()
{
    m_config.writeEntry("LastMimeType", currentMimeType().name());
}

void CreateDialog::loadConfiguration()
{
    m_config = KConfigGroup(KSharedConfig::openConfig()->group("CreateDialog"));
    QMimeType lastUsedMime = QMimeDatabase().mimeTypeForName(m_config.readEntry("LastMimeType", QStringLiteral("application/x-compressed-tar")));
    setMimeType(lastUsedMime.name());
}

QMimeType CreateDialog::currentMimeType() const
{
    Q_ASSERT(m_supportedMimeTypes.size() > m_ui->mimeComboBox->currentIndex());
    return QMimeDatabase().mimeTypeForName(m_supportedMimeTypes.at(m_ui->mimeComboBox->currentIndex()));
}

bool CreateDialog::setMimeType(const QString &mimeTypeName)
{
    int index = m_supportedMimeTypes.indexOf(mimeTypeName);
    if (index == -1) {
        return false;
    }
    m_ui->mimeComboBox->setCurrentIndex(index);

    // This is needed to make sure widgets get updated in case the mimetype is already selected.
    slotUpdateWidgets(index);

    return true;
}

}

#include "createdialog.moc"
