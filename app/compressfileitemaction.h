/*
    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>
    SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: GPL-2.0-or-later

*/

#ifndef COMPRESSFILEITEMACTION_H
#define COMPRESSFILEITEMACTION_H

#include <KAbstractFileItemActionPlugin>
#include <KFileItemListProperties>

class QAction;
class QWidget;


namespace Kerfuffle
{
class PluginManager;
}

class CompressFileItemAction : public KAbstractFileItemActionPlugin
{

Q_OBJECT

public:
    CompressFileItemAction(QObject* parent, const QVariantList& args);

    QList<QAction*> actions(const KFileItemListProperties& fileItemInfos, QWidget* parentWidget) override;

private:
    QAction *createAction(const QIcon& icon, const QString& name, QWidget *parent, const QList<QUrl>& urls, const QString& fileExtension);

    Kerfuffle::PluginManager *m_pluginManager;
};

#endif
