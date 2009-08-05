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
#include <QMenu>
#include <krun.h>
#include "kerfuffle/archive.h"
#include "kerfuffle/batchextract.h"

#include <KPluginFactory>
#include <KPluginLoader>
#include <KApplication>
#include <KLocale>
#include <kfileitemlistproperties.h>

using Kerfuffle::BatchExtract;

K_PLUGIN_FACTORY(ExtractHerePluginFactory,
                 registerPlugin<ExtractHereDndPlugin>();
                )
K_EXPORT_PLUGIN(ExtractHerePluginFactory("stupidname"))

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

    kDebug() << "plugin setup";
    QString extractHereMessage = i18n("Extract here");

    if (!Kerfuffle::supportedMimeTypes().contains(popupMenuInfo.mimeType())) {
        kDebug(1601) << "Unsupported file" << popupMenuInfo.mimeType() << Kerfuffle::supportedMimeTypes();
        return;
    }

    kDebug() << "Plugin executed";
    //<< popupMenuInfo.mimeGroup()
    //<< popupMenuInfo.mimeType();

    KAction *action = new KAction(KIcon("archive-extract"), extractHereMessage, NULL);
    connect(action, SIGNAL(triggered()),
            this, SLOT(slotTriggered()));

    userActions.append(action);
    m_dest = destination;
    m_urls = popupMenuInfo.urlList();

}

#include "extractHereDndPlugin.moc"
