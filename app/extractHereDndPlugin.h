/*
    SPDX-FileCopyrightText: 2009 Harald Hvaal <haraldhv (at@at) stud.ntnu.no>

    SPDX-License-Identifier: GPL-2.0-or-later
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
