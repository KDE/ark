/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
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
#include "infopanel.h"
#include "kerfuffle/archive.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QFileInfo>

#include <KLocale>
#include <KMimeType>
#include <KIconLoader>
#include <KIO/NetAccess>

using namespace Kerfuffle;

static QPixmap getMimeIcon(const QString& mimeName)
{
    return KIconLoader::global()->loadMimeTypeIcon(mimeName, KIconLoader::Desktop, KIconLoader::SizeHuge);
}

InfoPanel::InfoPanel(ArchiveModel *model, QWidget *parent)
        : QFrame(parent), m_model(model)
{
    setupUi(this);

    // Make the file name font bigger than the rest
    QFont fnt = fileName->font();
    if (fnt.pointSize() > -1) {
        fnt.setPointSize(fnt.pointSize() + 1);
    } else {
        fnt.setPixelSize(fnt.pixelSize() + 3);
    }
    fileName->setFont(fnt);

    updateWithDefaults();
}

InfoPanel::~InfoPanel()
{
}

void InfoPanel::updateWithDefaults()
{
    iconLabel->setPixmap(KIconLoader::global()->loadIcon(QLatin1String( "utilities-file-archiver" ), KIconLoader::Desktop, KIconLoader::SizeHuge));

    const QString currentFileName = prettyFileName();

    if (currentFileName.isEmpty()) {
        fileName->setText(i18n("No archive loaded"));
    } else {
        fileName->setText(currentFileName);
    }

    additionalInfo->setText(QString());
    hideMetaData();
    hideActions();
}

QString InfoPanel::prettyFileName() const
{
    if (m_prettyFileName.isEmpty()) {
        if (m_model->archive()) {
            QFileInfo fileInfo(m_model->archive()->fileName());
            return fileInfo.fileName();
        }
    }

    return m_prettyFileName;
}

void InfoPanel::setPrettyFileName(const QString& fileName)
{
    m_prettyFileName = fileName;
}

void InfoPanel::setIndex(const QModelIndex& index)
{
    if (!index.isValid()) {
        updateWithDefaults();
    } else {
        const ArchiveEntry& entry = m_model->entryForIndex(index);

        KMimeType::Ptr mimeType;

        if (entry[ IsDirectory ].toBool()) {
            mimeType = KMimeType::mimeType(QLatin1String( "inode/directory" ));
        } else {
            mimeType = KMimeType::findByPath(entry[ FileName ].toString(), 0, true);
        }

        iconLabel->setPixmap(getMimeIcon(mimeType->iconName()));
        if (entry[ IsDirectory ].toBool()) {
            int dirs;
            int files;
            const int children = m_model->childCount(index, dirs, files);
            additionalInfo->setText(KIO::itemsSummaryString(children, files, dirs, 0, false));
        } else if (entry.contains(Link)) {
            additionalInfo->setText(i18n("Symbolic Link"));
        } else {
            if (entry.contains(Size)) {
                additionalInfo->setText(KIO::convertSize(entry[ Size ].toULongLong()));
            } else {
                additionalInfo->setText(i18n("Unknown size"));

            }
        }

        const QStringList nameParts = entry[ FileName ].toString().split(QLatin1Char( '/' ), QString::SkipEmptyParts);
        const QString name = (nameParts.count() > 0) ? nameParts.last() : entry[ FileName ].toString();
        fileName->setText(name);

        metadataLabel->setText(metadataTextFor(index));
        showMetaData();
    }
}

void InfoPanel::setIndexes(const QModelIndexList &list)
{
    if (list.size() == 0) {
        setIndex(QModelIndex());
    } else if (list.size() == 1) {
        setIndex(list[ 0 ]);
    } else {
        iconLabel->setPixmap(KIconLoader::global()->loadIcon(QLatin1String( "utilities-file-archiver" ), KIconLoader::Desktop, KIconLoader::SizeHuge));
        fileName->setText(i18np("One file selected", "%1 files selected", list.size()));
        quint64 totalSize = 0;
        foreach(const QModelIndex& index, list) {
            const ArchiveEntry& entry = m_model->entryForIndex(index);
            totalSize += entry[ Size ].toULongLong();
        }
        additionalInfo->setText(KIO::convertSize(totalSize));
        hideMetaData();
    }
}

void InfoPanel::showMetaData()
{
    firstSeparator->show();
    metadataLabel->show();
}

void InfoPanel::hideMetaData()
{
    firstSeparator->hide();
    metadataLabel->hide();
}

void InfoPanel::showActions()
{
    secondSeparator->show();
    actionsLabel->show();
}

void InfoPanel::hideActions()
{
    secondSeparator->hide();
    actionsLabel->hide();
}

QString InfoPanel::metadataTextFor(const QModelIndex &index)
{
    const ArchiveEntry& entry = m_model->entryForIndex(index);
    QString text;

    KMimeType::Ptr mimeType;

    if (entry[ IsDirectory ].toBool()) {
        mimeType = KMimeType::mimeType(QLatin1String( "inode/directory" ));
    } else {
        mimeType = KMimeType::findByPath(entry[ FileName ].toString(), 0, true);
    }

    text += i18n("<b>Type:</b> %1<br/>",  mimeType->comment());

    if (entry.contains(Owner)) {
        text += i18n("<b>Owner:</b> %1<br/>", entry[ Owner ].toString());
    }

    if (entry.contains(Group)) {
        text += i18n("<b>Group:</b> %1<br/>", entry[ Group ].toString());
    }

    if (entry.contains(Link)) {
        text += i18n("<b>Target:</b> %1<br/>", entry[ Link ].toString());
    }

    if (entry.contains(IsPasswordProtected) && entry[ IsPasswordProtected ].toBool()) {
        text += i18n("<b>Password protected:</b> Yes<br/>");
    }

    return text;
}

#include "infopanel.moc"
