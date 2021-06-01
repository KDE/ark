/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>
 * Copyright (C) 2021 Alexander Lohnau <alexander.lohnau@gmx.de>
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

#include "compressfileitemaction.h"

#include <QMenu>
#include <QMimeDatabase>

#include <KDialogJobUiDelegate>
#include <KFileItem>
#include <KIO/ApplicationLauncherJob>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KService>

#include <algorithm>
#include <KIO/OpenFileManagerWindowJob>
#include <QDir>
#include <QGuiApplication>

#include "pluginmanager.h"
#include "addtoarchive.h"

K_PLUGIN_CLASS_WITH_JSON(CompressFileItemAction, "compressfileitemaction.json")

using namespace Kerfuffle;

CompressFileItemAction::CompressFileItemAction(QObject* parent, const QVariantList&)
    : KAbstractFileItemActionPlugin(parent)
    , m_pluginManager(new PluginManager(this))
{}

QList<QAction*> CompressFileItemAction::actions(const KFileItemListProperties& fileItemInfos, QWidget* parentWidget)
{
    // #268163: don't offer compression on already compressed archives, unless the user selected 2 or more of them.
    if (fileItemInfos.items().count() == 1 && m_pluginManager->supportedMimeTypes().contains(fileItemInfos.mimeType())) {
        return {};
    }

    // KFileItemListProperties::isLocal() doesn't check target URL (e.g. files on the desktop)
    const auto urlList = fileItemInfos.urlList();
    const bool hasLocalUrl = std::any_of(urlList.begin(), urlList.end(), [](const QUrl &url) {
        return url.isLocalFile();
    });

    if (!hasLocalUrl) {
        return {};
    }

    QList<QAction*> actions;
    const QIcon icon = QIcon::fromTheme(QStringLiteral("archive-insert"));

    QMenu *compressMenu = new QMenu(parentWidget);

    compressMenu->addAction(createAction(icon,
                                         i18nc("@action:inmenu Part of Compress submenu in Dolphin context menu", "Here (as TAR.GZ)"),
                                         parentWidget,
                                         urlList,
                                         QStringLiteral("tar.gz")));

    const QMimeType zipMime = QMimeDatabase().mimeTypeForName(QStringLiteral("application/zip"));
    // Don't offer zip compression if no zip plugin is available.
    if (!m_pluginManager->preferredWritePluginsFor(zipMime).isEmpty()) {
        compressMenu->addAction(createAction(icon,
                                             i18nc("@action:inmenu Part of Compress submenu in Dolphin context menu", "Here (as ZIP)"),
                                             parentWidget,
                                             urlList,
                                             QStringLiteral("zip")));
    }

    compressMenu->addAction(createAction(icon,
                                         i18nc("@action:inmenu Part of Compress submenu in Dolphin context menu", "Compress to..."),
                                         parentWidget,
                                         urlList,
                                         QString()));

    QAction *compressMenuAction = new QAction(i18nc("@action:inmenu Compress submenu in Dolphin context menu", "Compress"), parentWidget);
    compressMenuAction->setMenu(compressMenu);
    compressMenuAction->setEnabled(fileItemInfos.isLocal() && fileItemInfos.supportsWriting() && !m_pluginManager->availableWritePlugins().isEmpty());
    compressMenuAction->setIcon(icon);

    actions << compressMenuAction;
    return actions;
}

QAction *CompressFileItemAction::createAction(const QIcon& icon, const QString& name, QWidget *parent, const QList<QUrl>& urls, const QString& fileExtension)
{
    QAction *action = new QAction(icon, name, parent);

    connect(action, &QAction::triggered, this, [fileExtension, urls, name, parent, this]() {
        auto *addToArchiveJob = new AddToArchive(parent);
        addToArchiveJob->setChangeToFirstPath(true);
        for (const QUrl &url : urls) {
            addToArchiveJob->addInput(url);
        }
        if (!fileExtension.isEmpty()) {
            addToArchiveJob->setAutoFilenameSuffix(fileExtension);
        } else {
            if (!addToArchiveJob->showAddDialog()) {
                delete addToArchiveJob;
                return;
            }
        }
        addToArchiveJob->start();
        connect(addToArchiveJob, &KJob::finished, this, [this, addToArchiveJob](){
            if (addToArchiveJob->error() == 0) {
                KIO::highlightInFileManager({QUrl::fromLocalFile(addToArchiveJob->fileName())});
            } else if (!addToArchiveJob->errorString().isEmpty()) {
                Q_EMIT error(addToArchiveJob->errorString());
            }
        });
    });

    return action;
}

#include "compressfileitemaction.moc"
