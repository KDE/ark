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

#include "overwritedialog.h"

using namespace Kerfuffle;

OverwriteDialog::OverwriteDialog(QWidget *parent, const QList<const Archive::Entry*> &entries, const QHash<QString, QIcon> &icons, bool error)
        : QDialog(parent)
        , m_buttonBox(Qt::Horizontal)
        , m_okButton(i18n("OK"))
        , m_cancelButton(i18n("Cancel"))
{
    m_vBoxLayout.addLayout(&m_messageLayout);
    m_vBoxLayout.addWidget(&m_entriesList);
    m_vBoxLayout.addWidget(&m_buttonBox);

    m_messageLayout.addWidget(&m_messageIcon);
    m_messageLayout.addWidget(&m_messageText);

    m_messageIcon.setPixmap(QIcon::fromTheme(QStringLiteral("dialog-warning")).pixmap(QSize(64, 64)));
    if (error) {
        m_messageText.setText(i18n("Files with the following paths already exist. Remove them if you really want to overwrite."));
    } else {
        m_okButton.setIcon(QIcon::fromTheme(QStringLiteral("dialog-ok")));
        m_messageText.setText(i18n("Files with the following paths already exist. Do you want to continue overwriting them?"));
        m_buttonBox.addButton(&m_okButton, QDialogButtonBox::AcceptRole);
    }
    m_cancelButton.setIcon(QIcon::fromTheme(QStringLiteral("dialog-cancel")));
    m_buttonBox.addButton(&m_cancelButton, QDialogButtonBox::RejectRole);

    connect(&m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(&m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    foreach (const Archive::Entry *entry, entries) {
        QListWidgetItem *item = new QListWidgetItem(icons.value(entry->fullPath(true)), entry->fullPath(true));
        m_entriesList.addItem(item);
    }

    setLayout(&m_vBoxLayout);
    setFixedSize(window()->sizeHint());
}

OverwriteDialog::~OverwriteDialog()
{
}
