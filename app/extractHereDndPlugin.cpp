/*
    SPDX-FileCopyrightText: 2009 Harald Hvaal <haraldhv (at@at) stud.ntnu.no>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "extractHereDndPlugin.h"
#include "ark_debug.h"
#include "batchextract.h"
#include "archive_kerfuffle.h"
#include "pluginmanager.h"

#include <QAction>

#include <KFileItemListProperties>
#include <KPluginFactory>
#include <KLocalizedString>

K_PLUGIN_CLASS_WITH_JSON(ExtractHereDndPlugin, "ark_dndextract.json")

void ExtractHereDndPlugin::slotTriggered()
{
    qCDebug(ARK) << "Preparing job";
    BatchExtract *batchJob = new BatchExtract();

    batchJob->setAutoSubfolder(true);
    batchJob->setDestinationFolder(m_dest.toDisplayString(QUrl::PreferLocalFile));
    batchJob->setPreservePaths(true);
    for (const QUrl& url : std::as_const(m_urls)) {
        batchJob->addInput(url);
    }

    qCDebug(ARK) << "Starting job";
    batchJob->start();
}

ExtractHereDndPlugin::ExtractHereDndPlugin(QObject* parent, const QVariantList&)
        : KIO::DndPopupMenuPlugin(parent)
{
}

QList<QAction *> ExtractHereDndPlugin::setup(const KFileItemListProperties& popupMenuInfo,
                                             const QUrl& destination)
{
    QList<QAction *> actionList;

    Kerfuffle::PluginManager pluginManager;
    if (!pluginManager.supportedMimeTypes().contains(popupMenuInfo.mimeType())) {
        qCDebug(ARK) << popupMenuInfo.mimeType() << "is not a supported mimetype";
        return actionList;
    }

    qCDebug(ARK) << "Plugin executed";

    const QString extractHereMessage = i18nc("@action:inmenu Context menu shown when an archive is being drag'n'dropped", "Extract here");

    QAction *action = new QAction(QIcon::fromTheme(QStringLiteral("archive-extract")),
                                  extractHereMessage, nullptr);
    connect(action, &QAction::triggered, this, &ExtractHereDndPlugin::slotTriggered);

    actionList.append(action);
    m_dest = destination;
    m_urls = popupMenuInfo.urlList();

    return actionList;
}

#include "extractHereDndPlugin.moc"
