/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009,2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
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
#include "ui_adddialog.h"
#include "kerfuffle/archive_kerfuffle.h"

#include <KFilePlacesModel>
#include <KFileWidget>

#include <QDir>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QStandardItemModel>

namespace Kerfuffle
{
class AddDialogUI: public QWidget, public Ui::AddDialog
{
public:
    AddDialogUI(QWidget *parent = 0)
            : QWidget(parent) {
        setupUi(this);
    }
};

AddDialog::AddDialog(QWidget *parent,
                     const QString &caption,
                     const QUrl &startDir,
                     const QStringList &itemsToAdd)
        : CreateDialog(parent, caption, startDir)
{
    qCDebug(ARK) << "AddDialog loaded";

    m_ui = new AddDialogUI(this);
    m_vlayout->addWidget(m_ui);

    setupIconList(itemsToAdd);

    // Set up a default name if there's only one file to compress
    if (itemsToAdd.size() == 1) {
        const QFileInfo fileInfo(itemsToAdd.first());
        const QString fileName =
            fileInfo.isDir() ? fileInfo.dir().dirName() : fileInfo.baseName();
        m_fileWidget->setSelection(fileName);
    }
}

void AddDialog::setupIconList(const QStringList& itemsToAdd)
{
    QStandardItemModel* listModel = new QStandardItemModel(this);
    QStringList sortedList(itemsToAdd);

    sortedList.sort();

    Q_FOREACH(const QString& urlString, sortedList) {
        QUrl url(urlString);

        QStandardItem* item = new QStandardItem;
        item->setText(url.fileName());

        QMimeDatabase db;
        QString iconName = db.mimeTypeForUrl(url).iconName();
        item->setIcon(QIcon::fromTheme(iconName));

        item->setData(QVariant(url), KFilePlacesModel::UrlRole);

        listModel->appendRow(item);
    }

    m_ui->compressList->setModel(listModel);
}

}
