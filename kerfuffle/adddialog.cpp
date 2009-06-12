/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2008 Harald Hvaal <haraldhv atatatat stud.ntnu.no>
 * Copyright (C) 2009 Raphael Kubo da Costa <kubito@gmail.com>
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

#include "adddialog.h"
#include "ui_adddialog.h"
#include "kerfuffle/archive.h"

#include <KConfigGroup>
#include <KFilePlacesModel>
#include <KGlobal>

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

AddDialog::AddDialog(const QStringList& itemsToAdd,
                     const KUrl & startDir,
                     const QString & filter,
                     QWidget * parent,
                     QWidget * widget
                    )
        : KFileDialog(startDir, filter, parent, widget)
{
    setOperationMode(KFileDialog::Saving);
    setMode(KFile::File | KFile::LocalOnly);
    setCaption(i18n("Compress to Archive"));

    loadConfiguration();

    connect(this, SIGNAL(okClicked()), SLOT(updateDefaultMimeType()));

    m_ui = new AddDialogUI(this);
    mainWidget()->layout()->addWidget(m_ui);

    setupIconList(itemsToAdd);

    //These extra options will be implemented in a 4.2+ version of
    //ark
    m_ui->groupExtraOptions->hide();
}

void AddDialog::loadConfiguration()
{
    m_config = KConfigGroup(KGlobal::config()->group("AddDialog"));

    QString defaultMimeType = "application/x-compressed-tar";
    QStringList writeMimeTypes = Kerfuffle::supportedWriteMimeTypes();
    QString lastMimeType = m_config.readEntry("LastMimeType", defaultMimeType);

    if (writeMimeTypes.contains(lastMimeType))
        setMimeFilter(writeMimeTypes, lastMimeType);
    else
        setMimeFilter(writeMimeTypes, defaultMimeType);
}

void AddDialog::setupIconList(const QStringList& itemsToAdd)
{
    QStandardItemModel* listModel = new QStandardItemModel(this);
    QStringList sortedList(itemsToAdd);

    sortedList.sort();

    Q_FOREACH(const QString& urlString, sortedList) {
        KUrl url(urlString);

        QStandardItem* item = new QStandardItem;
        item->setText(url.fileName());

        QString iconName = KMimeType::iconNameForUrl(url);
        item->setIcon(KIcon(iconName));

        item->setData(QVariant(url), KFilePlacesModel::UrlRole);

        listModel->appendRow(item);
    }

    m_ui->compressList->setModel(listModel);
}

void AddDialog::updateDefaultMimeType()
{
    m_config.writeEntry("LastMimeType", currentMimeFilter());
}
}

#include "adddialog.moc"
