/*
    SPDX-FileCopyrightText: 2007 Henrique Pinto <henrique.pinto@kdemail.net>

    SPDX-License-Identifier: GPL-2.0-or-later

*/

#include "infopanel.h"
#include "archiveentry.h"

#include <KIO/Global>
#include <KLocalizedString>

#include <QFileInfo>
#include <QIcon>
#include <QMimeDatabase>

using namespace Kerfuffle;

QPixmap InfoPanel::getPixmap(const QString& name)
{
    return QIcon::fromTheme(name).pixmap(48);
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
    iconLabel->setPixmap(getPixmap(QStringLiteral("utilities-file-archiver")));

    const QString currentFileName = prettyFileName();

    if (currentFileName.isEmpty()) {
        fileName->setText(i18n("No archive loaded"));
    } else {
        fileName->setText(currentFileName);
    }

    additionalInfo->setText(QString());
    hideMetaData();
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
        const Archive::Entry *entry = m_model->entryForIndex(index);
        if (!entry) {
            return;
        }

        QMimeDatabase db;
        QMimeType mimeType;
        if (entry->isDir()) {
            mimeType = db.mimeTypeForName(QStringLiteral("inode/directory"));
        } else {
            mimeType = db.mimeTypeForFile(entry->fullPath(), QMimeDatabase::MatchExtension);
        }

        iconLabel->setPixmap(getPixmap(mimeType.iconName()));
        if (entry->isDir()) {
            uint dirs;
            uint files;
            entry->countChildren(dirs, files);
            additionalInfo->setText(KIO::itemsSummaryString(dirs + files, files, dirs, 0, false));
        } else if (!entry->property("link").toString().isEmpty()) {
            additionalInfo->setText(i18n("Symbolic Link"));
        } else {
            if (entry->property("size") != 0) {
                additionalInfo->setText(KIO::convertSize(entry->property("size").toULongLong()));
            } else {
                additionalInfo->setText(i18n("Unknown size"));

            }
        }

        const QStringList nameParts = entry->fullPath().split(QLatin1Char( '/' ), Qt::SkipEmptyParts);
        const QString name = (nameParts.count() > 0) ? nameParts.last() : entry->fullPath();
        fileName->setText(name);

        showMetaDataFor(index);
    }
}

void InfoPanel::setIndexes(const QModelIndexList &list)
{
    if (list.size() == 0) {
        setIndex(QModelIndex());
    } else if (list.size() == 1) {
        setIndex(list[ 0 ]);
    } else {
        iconLabel->setPixmap(getPixmap(QStringLiteral("utilities-file-archiver")));
        fileName->setText(i18np("One file selected", "%1 files selected", list.size()));
        quint64 totalSize = 0;
        for (const QModelIndex& index : list) {
            const Archive::Entry *entry = m_model->entryForIndex(index);
            totalSize += entry->property("size").toULongLong();
        }
        additionalInfo->setText(KIO::convertSize(totalSize));
        hideMetaData();
    }
}

void InfoPanel::showMetaData()
{
    m_separator->show();
    m_metaDataWidget->show();
}

void InfoPanel::hideMetaData()
{
    m_separator->hide();
    m_metaDataWidget->hide();
}

void InfoPanel::showMetaDataFor(const QModelIndex &index)
{
    showMetaData();

    const Archive::Entry *entry = m_model->entryForIndex(index);

    QMimeDatabase db;
    QMimeType mimeType;

    if (entry->isDir()) {
        mimeType = db.mimeTypeForName(QStringLiteral("inode/directory"));
    } else {
        mimeType = db.mimeTypeForFile(entry->fullPath(), QMimeDatabase::MatchExtension);
    }

    if (entry->isExecutable() && mimeType.isDefault()) {
        m_typeValueLabel->setText(db.mimeTypeForName(QStringLiteral("application/x-executable")).comment());
    } else {
        m_typeValueLabel->setText(mimeType.comment());
    }

    if (!entry->property("owner").toString().isEmpty()) {
        m_ownerLabel->show();
        m_ownerValueLabel->show();
        m_ownerValueLabel->setText(entry->property("owner").toString());
    } else {
        m_ownerLabel->hide();
        m_ownerValueLabel->hide();
    }

    if (!entry->property("group").toString().isEmpty()) {
        m_groupLabel->show();
        m_groupValueLabel->show();
        m_groupValueLabel->setText(entry->property("group").toString());
    } else {
        m_groupLabel->hide();
        m_groupValueLabel->hide();
    }

    if (!entry->property("link").toString().isEmpty()) {
        m_targetLabel->show();
        m_targetValueLabel->show();
        m_targetValueLabel->setText(entry->property("link").toString());
    } else {
        m_targetLabel->hide();
        m_targetValueLabel->hide();
    }

    if (entry->property("isPasswordProtected").toBool()) {
        m_passwordLabel->show();
        m_passwordValueLabel->show();
    } else {
        m_passwordLabel->hide();
        m_passwordValueLabel->hide();
    }
}
