/*
 * ark -- archiver for the KDE project
 *
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

#include "adddialog.h"
#include "ark_debug.h"
#include "archiveformat.h"
#include "compressionoptionswidget.h"
#include "mimetypes.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

namespace Kerfuffle
{

AddDialog::AddDialog(QWidget *parent,
                     const QString &title,
                     const QUrl &startDir,
                     const QMimeType &mimeType,
                     const CompressionOptions &opts)
        : QDialog(parent, Qt::Dialog)
        , m_mimeType(mimeType)
        , m_compOptions(opts)
{
    qCDebug(ARK) << "AddDialog loaded with options:" << m_compOptions;

    setWindowTitle(title);

    QVBoxLayout *vlayout = new QVBoxLayout(this);
    m_fileWidget = new KFileWidget(startDir, this);
    vlayout->addWidget(m_fileWidget);

    QPushButton *optionsButton = new QPushButton(QIcon::fromTheme(QStringLiteral("settings-configure")),
                                                 i18n("Advanced Options"));
    optionsButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_fileWidget->setCustomWidget(optionsButton);

    connect(optionsButton, &QPushButton::clicked, this, &AddDialog::slotOpenOptions);

    m_fileWidget->setMode(KFile::Files | KFile::Directory | KFile::LocalOnly | KFile::ExistingOnly);
    m_fileWidget->setOperationMode(KFileWidget::Opening);

    m_fileWidget->okButton()->setText(i18nc("@action:button", "Add"));
    m_fileWidget->okButton()->show();
    connect(m_fileWidget->okButton(), &QPushButton::clicked, m_fileWidget, &KFileWidget::slotOk);
    connect(m_fileWidget, &KFileWidget::accepted, m_fileWidget, &KFileWidget::accept);
    connect(m_fileWidget, &KFileWidget::accepted, this, &QDialog::accept);

    m_fileWidget->cancelButton()->show();
    connect(m_fileWidget->cancelButton(), &QPushButton::clicked, this, &QDialog::reject);
}

AddDialog::~AddDialog()
{
}

QStringList AddDialog::selectedFiles() const
{
    return m_fileWidget->selectedFiles();
}

CompressionOptions AddDialog::compressionOptions() const
{
    qCDebug(ARK) << "Returning with options:" << m_compOptions;
    return m_compOptions;
}

void AddDialog::slotOpenOptions()
{
    optionsDialog = new QDialog(this);
    QVBoxLayout *vlayout = new QVBoxLayout(optionsDialog);
    optionsDialog->setWindowTitle(i18n("Advanced Options"));

    CompressionOptionsWidget *optionsWidget = new CompressionOptionsWidget(optionsDialog, m_compOptions);
    optionsWidget->setMimeType(m_mimeType);
    optionsWidget->setEncryptionVisible(false);
    optionsWidget->collapsibleMultiVolume->setVisible(false);
    optionsWidget->collapsibleCompression->expand();
    vlayout->addWidget(optionsWidget);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, optionsDialog);
    vlayout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, optionsDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, optionsDialog, &QDialog::reject);

    optionsDialog->layout()->setSizeConstraint(QLayout::SetFixedSize);

    connect(optionsDialog, &QDialog::finished, this, [this, optionsWidget](int result) {
        if (result == QDialog::Accepted) {
            m_compOptions = optionsWidget->commpressionOptions();
        }
        optionsDialog->deleteLater();
    });

    optionsDialog->open();
}

}
