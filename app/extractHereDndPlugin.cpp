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
#include "kerfuffle/archive_kerfuffle.h"

#include <QAction>
#include <QDebug>
#include <KPluginFactory>
#include <KPluginLoader>
#include <KLocale>
#include <kfileitemlistproperties.h>
#include <KUrl>
K_PLUGIN_FACTORY(ExtractHerePluginFactory,
                 registerPlugin<ExtractHereDndPlugin>();
                )
K_EXPORT_PLUGIN(ExtractHerePluginFactory("stupidname", "ark"))

void ExtractHereDndPlugin::slotTriggered()
{
    //qDebug() << "Preparing job";
    BatchExtract *batchJob = new BatchExtract();

    batchJob->setAutoSubfolder(true);
    batchJob->setDestinationFolder(m_dest.pathOrUrl());
    batchJob->setPreservePaths(true);
    foreach(const QUrl& url, m_urls) {
        batchJob->addInput(url);
    }

    batchJob->start();
    //qDebug() << "Started job";

}

ExtractHereDndPlugin::ExtractHereDndPlugin(QObject* parent, const QVariantList&)
        : KonqDndPopupMenuPlugin(parent)
{
}

void ExtractHereDndPlugin::setup(const KFileItemListProperties& popupMenuInfo,
                                 QUrl destination,
                                 QList<QAction*>& userActions)
{
    const QString extractHereMessage = i18nc("@action:inmenu Context menu shown when an archive is being drag'n'dropped", "Extract here");

    if (!Kerfuffle::supportedMimeTypes().contains(popupMenuInfo.mimeType())) {
        //qDebug() << popupMenuInfo.mimeType() << "is not a supported mimetype";
        return;
    }

    //qDebug() << "Plugin executed";

    QAction *action = new QAction(QIcon::fromTheme(QLatin1String("archive-extract")),
                                  extractHereMessage, NULL);
    connect(action, &QAction::triggered, this, &ExtractHereDndPlugin::slotTriggered);

    userActions.append(action);
    m_dest = destination;
    m_urls = popupMenuInfo.urlList();
}

#include "extractHereDndPlugin.moc"
