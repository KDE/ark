/*
    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>
    SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef EXTRACTFILEITEMACTION_H
#define EXTRACTFILEITEMACTION_H

#include <KAbstractFileItemActionPlugin>
#include <KFileItemListProperties>

class QAction;
class QWidget;


namespace Kerfuffle
{
class PluginManager;
}

class ExtractFileItemAction : public KAbstractFileItemActionPlugin
{

Q_OBJECT

public:
    ExtractFileItemAction(QObject* parent, const QVariantList& args);

    QList<QAction*> actions(const KFileItemListProperties& fileItemInfos, QWidget* parentWidget) override;

private:
    enum AdditionalJobOptions {
        None,
        ShowDialog,
        AutoSubfolder,
    };
    QAction *createAction(const QIcon& icon, const QString& name, QWidget *parent, const QList<QUrl>& urls, AdditionalJobOptions option);

    Kerfuffle::PluginManager *m_pluginManager;
};

#endif
