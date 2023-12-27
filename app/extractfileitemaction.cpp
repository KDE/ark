/*
    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>
    SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "extractfileitemaction.h"

#include <QFileInfo>
#include <QMenu>

#include <KIO/CopyJob>
#include <KIO/JobUiDelegate>
#include <KIO/OpenFileManagerWindowJob>
#include <KLocalizedString>
#include <KPluginFactory>

#include "batchextract.h"
#include "mimetypes.h"
#include "pluginmanager.h"
#include "settings.h"

K_PLUGIN_CLASS_WITH_JSON(ExtractFileItemAction, "extractfileitemaction.json")

using namespace Kerfuffle;

ExtractFileItemAction::ExtractFileItemAction(QObject *parent, const QVariantList &)
    : KAbstractFileItemActionPlugin(parent)
    , m_pluginManager(new PluginManager(this))
{
}

QList<QAction *> ExtractFileItemAction::actions(const KFileItemListProperties &fileItemInfos, QWidget *parentWidget)
{
    QList<QAction *> actions;
    const QIcon icon = QIcon::fromTheme(QStringLiteral("archive-extract"));

    bool readOnlyParentDir = false;
    QList<QUrl> supportedUrls;
    // Filter URLs by supported mimetypes.
    const auto urlList = fileItemInfos.urlList();
    for (const QUrl &url : urlList) {
        if (!url.isLocalFile()) {
            continue;
        }
        const QMimeType mimeType = determineMimeType(url.toLocalFile());
        if (m_pluginManager->preferredPluginsFor(mimeType).isEmpty()) {
            continue;
        }
        supportedUrls << url;
        // Check whether we can write in the parent directory of the file.
        if (!readOnlyParentDir) {
            const QString directory = url.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash).toLocalFile();
            if (!QFileInfo(directory).isWritable()) {
                readOnlyParentDir = true;
            }
        }
    }

    if (supportedUrls.isEmpty()) {
        return {};
    }

    QAction *extractToAction = createAction(icon,
                                            i18nc("@action:inmenu Part of Extract submenu in Dolphin context menu", "Extract to…"),
                                            parentWidget,
                                            supportedUrls,
                                            AdditionalJobOptions::ShowDialog);

    // #189177: disable "extract here" actions in read-only folders.
    if (readOnlyParentDir) {
        actions << extractToAction;
    } else {
        QMenu *extractMenu = new QMenu(parentWidget);

        extractMenu->addAction(createAction(icon,
                                            i18nc("@action:inmenu Part of Extract submenu in Dolphin context menu", "Extract here"),
                                            parentWidget,
                                            supportedUrls,
                                            AdditionalJobOptions::None));

        extractMenu->addAction(createAction(QIcon::fromTheme(QStringLiteral("archive-remove")),
                                            i18nc("@action:inmenu Part of Extract submenu in Dolphin context menu", "Extract here and delete archive"),
                                            parentWidget,
                                            supportedUrls,
                                            AdditionalJobOptions::AutoDelete));

        extractMenu->addAction(extractToAction);

        QAction *extractMenuAction = new QAction(i18nc("@action:inmenu Extract submenu in Dolphin context menu", "Extract"), parentWidget);
        extractMenuAction->setMenu(extractMenu);
        extractMenuAction->setIcon(icon);

        actions << extractMenuAction;
    }

    return actions;
}

QAction *ExtractFileItemAction::createAction(const QIcon &icon, const QString &name, QWidget *parent, const QList<QUrl> &urls, AdditionalJobOptions option)
{
    QAction *action = new QAction(icon, name, parent);
    connect(action, &QAction::triggered, this, [urls, option, this]() {
        // Don't pass a parent to the job, otherwise it will be killed if dolphin gets closed.
        auto *batchExtractJob = new BatchExtract(nullptr);
        batchExtractJob->setDestinationFolder(QFileInfo(urls.first().toLocalFile()).path());
        batchExtractJob->setOpenDestinationAfterExtraction(ArkSettings::openDestinationFolderAfterExtraction());
        if (option == ShowDialog) {
            if (!batchExtractJob->showExtractDialog()) {
                delete batchExtractJob;
                return;
            }
        } else {
            batchExtractJob->setAutoSubfolder(true);
        }
        for (const QUrl &url : urls) {
            batchExtractJob->addInput(url);
        }
        batchExtractJob->start();
        connect(batchExtractJob, &KJob::finished, this, [this, batchExtractJob, option, urls]() {
            if (batchExtractJob->error()) {
                Q_EMIT error(batchExtractJob->errorString());
            } else if (option == AutoDelete) {
                KIO::Job *job = KIO::trash(urls);
                job->uiDelegate()->setAutoErrorHandlingEnabled(true);
            }
            batchExtractJob->deleteLater();
        });
    });
    return action;
}

#include "extractfileitemaction.moc"
#include "moc_extractfileitemaction.cpp"
