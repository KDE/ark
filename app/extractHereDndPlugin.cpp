/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Harald Hvaal <haraldhv (at@at) stud.ntnu.no>
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

#include "extractHereDndPlugin.h"
#include "batchextract.h"
#include "kerfuffle/archive.h"

#include <KAction>
#include <KDebug>
#include <KPluginFactory>
#include <KPluginLoader>
#include <KLocale>
#include <kfileitemlistproperties.h>

K_PLUGIN_FACTORY(ExtractHerePluginFactory,
                 registerPlugin<ExtractHereDndPlugin>();
                )
K_EXPORT_PLUGIN(ExtractHerePluginFactory("stupidname", "ark"))

void ExtractHereDndPlugin::slotTriggered()
{
    kDebug() << "Preparing job";
    BatchExtract *batchJob = new BatchExtract();

    batchJob->setAutoSubfolder(true);
    batchJob->setDestinationFolder(m_dest.pathOrUrl());
    batchJob->setPreservePaths(true);
    foreach(const KUrl& url, m_urls) {
        batchJob->addInput(url);
    }

    batchJob->start();
    kDebug() << "Started job";

}

ExtractHereDndPlugin::ExtractHereDndPlugin(QObject* parent, const QVariantList&)
        : KonqDndPopupMenuPlugin(parent)
{
}

void ExtractHereDndPlugin::setup(const KFileItemListProperties& popupMenuInfo,
                                 KUrl destination,
                                 QList<QAction*>& userActions)
{
    const QString extractHereMessage = i18nc("@action:inmenu Context menu shown when an archive is being drag'n'dropped", "Extract here");

    if (!Kerfuffle::supportedMimeTypes().contains(popupMenuInfo.mimeType())) {
        kDebug() << popupMenuInfo.mimeType() << "is not a supported mimetype";
        return;
    }

    kDebug() << "Plugin executed";

    KAction *action = new KAction(KIcon(QLatin1String("archive-extract")),
                                  extractHereMessage, NULL);
    connect(action, SIGNAL(triggered()), this, SLOT(slotTriggered()));

    userActions.append(action);
    m_dest = destination;
    m_urls = popupMenuInfo.urlList();
}

#include "extractHereDndPlugin.moc"
