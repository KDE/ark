/*
    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>
    SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "compressfileitemaction.h"

#include <QDir>
#include <QMenu>
#include <QMimeDatabase>

#include <KFileItem>
#include <KIO/ApplicationLauncherJob>
#include <KIO/StatJob>
#include <KLocalizedString>
#include <KPluginFactory>

#include <KIO/OpenFileManagerWindowJob>
#include <algorithm>

#include "addtoarchive.h"
#include "pluginmanager.h"

K_PLUGIN_CLASS_WITH_JSON(CompressFileItemAction, "compressfileitemaction.json")

using namespace Kerfuffle;

CompressFileItemAction::CompressFileItemAction(QObject *parent, const QVariantList &)
    : KAbstractFileItemActionPlugin(parent)
    , m_pluginManager(new PluginManager(this))
{
}

QList<QAction *> CompressFileItemAction::actions(const KFileItemListProperties &fileItemInfos, QWidget *parentWidget)
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

    QList<QAction *> actions;
    QList<QAction *> actionsToBeDisabledInReadOnlyDir;
    const QIcon icon = QIcon::fromTheme(QStringLiteral("archive-insert"));

    QMenu *compressMenu = new QMenu(parentWidget);

    compressMenu->addAction(createAction(icon, parentWidget, urlList, QStringLiteral("tar.gz")));
    actionsToBeDisabledInReadOnlyDir << compressMenu->actions().last();

    const QMimeType zipMime = QMimeDatabase().mimeTypeForName(QStringLiteral("application/zip"));
    // Don't offer zip compression if no zip plugin is available.
    if (!m_pluginManager->preferredWritePluginsFor(zipMime).isEmpty()) {
        compressMenu->addAction(createAction(icon, parentWidget, urlList, QStringLiteral("zip")));
        actionsToBeDisabledInReadOnlyDir << compressMenu->actions().last();
    }

    compressMenu->addAction(createAction(icon, parentWidget, urlList, QString()));

    QAction *compressMenuAction = new QAction(i18nc("@action:inmenu Compress submenu in Dolphin context menu", "Compress"), parentWidget);
    compressMenuAction->setMenu(compressMenu);
    compressMenuAction->setEnabled(fileItemInfos.isLocal() && !m_pluginManager->availableWritePlugins().isEmpty());
    compressMenuAction->setIcon(icon);

    if (compressMenuAction->isEnabled()) {
        const KFileItem &first = fileItemInfos.items().first();
        auto *job = KIO::stat(first.url().adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash));
        connect(job, &KJob::result, compressMenu, [actionsToBeDisabledInReadOnlyDir, job]() {
            if (!job->error() && !KFileItem(job->statResult(), job->url()).isWritable()) {
                for (auto action : actionsToBeDisabledInReadOnlyDir) {
                    action->setEnabled(false);
                }
            }
        });
    }

    actions << compressMenuAction;
    return actions;
}

QAction *CompressFileItemAction::createAction(const QIcon &icon, QWidget *parent, const QList<QUrl> &urls, const QString &fileExtension)
{
    QString name;
    if (fileExtension.isEmpty()) {
        name = i18nc("@action:inmenu Part of Compress submenu in Dolphin context menu", "Compress to...");
    } else {
        QString fileName = AddToArchive::getFileNameForUrls(urls, fileExtension).section(QDir::separator(), -1);
        if (fileName.length() > 21) {
            fileName = fileName.left(10) + QStringLiteral("…") + fileName.right(10);
        }

        fileName.replace(QStringLiteral("&"), QStringLiteral("&&"));
        name = i18nc("@action:inmenu Part of Compress submenu in Dolphin context menu, %1 filename", "Compress to \"%1\"", fileName);
    }

    QAction *action = new QAction(icon, name, parent);

    connect(action, &QAction::triggered, this, [fileExtension, urls, parent, this]() {
        // Don't pass a parent to the job, otherwise it will be killed if dolphin gets closed.
        auto *addToArchiveJob = new AddToArchive(nullptr);
        addToArchiveJob->setImmediateProgressReporting(true);
        addToArchiveJob->setChangeToFirstPath(true);
        for (const QUrl &url : urls) {
            addToArchiveJob->addInput(url);
        }
        if (!fileExtension.isEmpty()) {
            addToArchiveJob->setAutoFilenameSuffix(fileExtension);
        } else {
            if (!addToArchiveJob->showAddDialog(parent)) {
                delete addToArchiveJob;
                return;
            }
        }
        addToArchiveJob->start();
        connect(addToArchiveJob, &KJob::finished, this, [this, addToArchiveJob]() {
            if (addToArchiveJob->error() == 0) {
                KIO::highlightInFileManager({QUrl::fromLocalFile(addToArchiveJob->fileName())});
            } else if (!addToArchiveJob->errorString().isEmpty()) {
                Q_EMIT error(addToArchiveJob->errorString());
            }
            addToArchiveJob->deleteLater();
        });
    });

    return action;
}

#include "compressfileitemaction.moc"
#include "moc_compressfileitemaction.cpp"
