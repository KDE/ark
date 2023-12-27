/*
    SPDX-FileCopyrightText: 2016 Ragnar Thomsen <rthomsen6@gmail.com>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "overwritedialog.h"

#include <KLocalizedString>

using namespace Kerfuffle;

OverwriteDialog::OverwriteDialog(QWidget *parent, const QList<const Archive::Entry *> &entries, bool error)
    : QDialog(parent)
    , m_buttonBox(QDialogButtonBox::Cancel, Qt::Horizontal)
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
        m_messageText.setText(i18n("Files with the following paths already exist. Do you want to continue overwriting them?"));
        m_buttonBox.addButton(QDialogButtonBox::Ok);
    }

    connect(&m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(&m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    for (const Archive::Entry *entry : entries) {
        QListWidgetItem *item = new QListWidgetItem(entry->icon(), entry->fullPath(NoTrailingSlash));
        m_entriesList.addItem(item);
    }

    setLayout(&m_vBoxLayout);
    setFixedSize(window()->sizeHint());
}

OverwriteDialog::~OverwriteDialog()
{
}

#include "moc_overwritedialog.cpp"
