/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Harald Hvaal <haraldhv (at@at) stud.ntnu.no>
 * Copyright (C) 2012 basysKom GmbH <info@basyskom.com>
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
#include "kerfuffle/archive.h"
#include "kerfuffle/archiveinterface.h"
#include "kerfuffle/batchextract.h"

#include <KAction>
#include <KDebug>
#include <KPluginFactory>
#include <KPluginLoader>
#include <KLocale>
#include <KMimeType>
#include <kfileitemlistproperties.h>

K_PLUGIN_FACTORY(ExtractHerePluginFactory,
                 registerPlugin<ExtractHereDndPlugin>();
                )
K_EXPORT_PLUGIN(ExtractHerePluginFactory("stupidname", "ark"))

void ExtractHereDndPlugin::slotTriggered()
{
    kDebug(1601) << "Preparing job";
    Kerfuffle::BatchExtract *batchJob = new Kerfuffle::BatchExtract();

    Kerfuffle::ExtractionOptions options;
    options[QLatin1String("AutoSubfolders")] = true;
    options[QLatin1String("OpenDestinationAfterExtraction")] = false;
    options[QLatin1String("CloseArkAfterExtraction")] = true;
    options[QLatin1String("FixFileNameEncoding")] = true;
    options[QLatin1String("MultiThreadingEnabled")] = false;
    options[QLatin1String("PreservePaths")] = false;
    options[QLatin1String("RenameSupported")] = false;
    options[QLatin1String("ConflictsHandling")] = (int)Kerfuffle::AlwaysAsk;
    options[QLatin1String("DestinationDirectory")] = QVariant(m_dest.pathOrUrl());
    options[QLatin1String("TestBeforeExtraction")] = true;

    KMimeType::Ptr mime;
    QList<int> supportedOptions;
    foreach(const KUrl & url, m_urls) {
        batchJob->addInput(url);
        mime = KMimeType::findByUrl(url);
        if (mime && Kerfuffle::supportedMimeTypes().contains(mime->name())) {
            supportedOptions = Kerfuffle::supportedOptions(mime->name());
            if (supportedOptions.contains(Kerfuffle::PreservePath)) {
                options[QLatin1String("PreservePathsEnabled")] = true;
            }

            if (supportedOptions.contains(Kerfuffle::Rename)) {
                options[QLatin1String("RenameSupported")] = true;
            }
        }
    }
    batchJob->setOptions(options);

    batchJob->start();
    kDebug(1601) << "Started job";

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
        kDebug(1601) << popupMenuInfo.mimeType() << "is not a supported mimetype";
        return;
    }

    kDebug(1601) << "Plugin executed";

    KAction *action = new KAction(KIcon(QLatin1String("archive-extract")),
                                  extractHereMessage, NULL);
    connect(action, SIGNAL(triggered()), this, SLOT(slotTriggered()));

    userActions.append(action);
    m_dest = destination;
    m_urls = popupMenuInfo.urlList();
}

#include "extractHereDndPlugin.moc"
