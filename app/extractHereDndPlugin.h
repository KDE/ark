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

#ifndef EXTRACTHEREDNDPLUGIN_H
#define EXTRACTHEREDNDPLUGIN_H

#include <KIO/DndPopupMenuPlugin>

#include <QUrl>

class ExtractHereDndPlugin : public KIO::DndPopupMenuPlugin
{
    Q_OBJECT

private Q_SLOTS:
    void slotTriggered();

public:
    ExtractHereDndPlugin(QObject* parent, const QVariantList&);

    QList<QAction *> setup(const KFileItemListProperties& popupMenuInfo,
                                   const QUrl& destination) override;

private:
    QUrl m_dest;
    QList<QUrl> m_urls;
};

#endif /* EXTRACTHEREDNDPLUGIN_H */
