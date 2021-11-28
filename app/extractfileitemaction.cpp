/*
    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>
    SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "extractfileitemaction.h"

#include <QFileInfo>
#include <QMenu>

#include <KIO/OpenFileManagerWindowJob>
#include <KLocalizedString>
#include <KPluginFactory>

#include "batchextract.h"
#include "jobs.h"
#include "mimetypes.h"
#include "pluginmanager.h"
#include "settings.h"

K_PLUGIN_CLASS_WITH_JSON(ExtractFileItemAction, "extractfileitemaction.json")

using namespace Kerfuffle;

ExtractFileItemAction::ExtractFileItemAction(QObject* parent, const QVariantList&)
    : KAbstractFileItemActionPlugin(parent)
    , m_pluginManager(new PluginManager(this))
{}

QList<QAction*> ExtractFileItemAction::actions(const KFileItemListProperties& fileItemInfos, QWidget* parentWidget)
{
    QList<QAction*> actions;
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
                                            i18nc("@action:inmenu Part of Extract submenu in Dolphin context menu", "Extract archive to..."),
                                            parentWidget,
                                            supportedUrls,
                                            AdditionalJobOption::ShowDialog);

    // #189177: disable "extract here" actions in read-only folders.
    if (readOnlyParentDir) {
       actions << extractToAction;
    } else {
        QMenu *extractMenu = new QMenu(parentWidget);

        extractMenu->addAction(createAction(icon,
                                            i18nc("@action:inmenu Part of Extract submenu in Dolphin context menu", "Extract archive here"),
                                            parentWidget,
                                            supportedUrls,
                                            AdditionalJobOption::AllowRetryPassword));

        extractMenu->addAction(extractToAction);

        extractMenu->addAction(
            createAction(icon,
                         i18nc("@action:inmenu Part of Extract submenu in Dolphin context menu", "Extract archive here, autodetect subfolder"),
                         parentWidget,
                         supportedUrls,
                         AdditionalJobOption::AutoSubfolder | AdditionalJobOption::AllowRetryPassword));

        QAction *extractMenuAction = new QAction(i18nc("@action:inmenu Extract submenu in Dolphin context menu", "Extract"), parentWidget);
        extractMenuAction->setMenu(extractMenu);
        extractMenuAction->setIcon(icon);

        actions << extractMenuAction;
    }

    return actions;
}

void ExtractFileItemAction::slotActionTriggered()
{
    auto action = qobject_cast<QAction *>(sender());
    if (!action) {
        return;
    }

    const auto urls = action->property("urls").value<QVariantList>();
    const auto option = static_cast<AdditionalJobOptions>(action->property("option").toInt());
    const bool incorrectTryAgain = action->property("incorrectTryAgain").toBool();

    auto batchExtractJob = new BatchExtract(action->parent());
    batchExtractJob->setDestinationFolder(QFileInfo(urls.first().toUrl().toLocalFile()).path());
    batchExtractJob->setOpenDestinationAfterExtraction(ArkSettings::openDestinationFolderAfterExtraction());
    batchExtractJob->setIncorrectTryAgain(incorrectTryAgain);

    if (option & AdditionalJobOption::AutoSubfolder) {
        batchExtractJob->setAutoSubfolder(true);
    } else if (option & AdditionalJobOption::ShowDialog) {
        if (!batchExtractJob->showExtractDialog()) {
            delete batchExtractJob;
            return;
        }
    }

    for (const QVariant &url : urls) {
        batchExtractJob->addInput(url.toUrl());
    }

    batchExtractJob->start();
    connect(batchExtractJob, &KJob::finished, this, [this, batchExtractJob, action, option]() {
        if (batchExtractJob->error() == KJob::NoError) {
            return;
        }

        if (batchExtractJob->error() == Job::PasswordError && (option & AdditionalJobOption::AllowRetryPassword)) {
            // If AllowRetryPassword is set, allow the user to type a new password if the entered password is wrong.
            action->setProperty("incorrectTryAgain", true);
            action->trigger();
        } else {
            Q_EMIT error(batchExtractJob->errorString());
        }
    });
}

QAction *ExtractFileItemAction::createAction(const QIcon &icon, const QString &name, QWidget *parent, const QList<QUrl> &urls, AdditionalJobOptions option)
{
    QAction *action = new QAction(icon, name, parent);

    action->setProperty("urls", QVariantList(urls.cbegin(), urls.cend()));
    action->setProperty("option", static_cast<int>(option));
    action->setProperty("incorrectTryAgain", false);

    connect(action, &QAction::triggered, this, &ExtractFileItemAction::slotActionTriggered);
    return action;
}

#include "extractfileitemaction.moc"
