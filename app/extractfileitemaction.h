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
    enum class AdditionalJobOption {
        None = 1 << 0,
        ShowDialog = 1 << 1,
        AutoSubfolder = 1 << 2,
        AllowRetryPassword = 1 << 3, /**< If the entered password is wrong, allow the user to retry rather than quit directly */
    };
    Q_DECLARE_FLAGS(AdditionalJobOptions, AdditionalJobOption)

    ExtractFileItemAction(QObject* parent, const QVariantList& args);

    QList<QAction*> actions(const KFileItemListProperties& fileItemInfos, QWidget* parentWidget) override;

private Q_SLOTS:
    void slotActionTriggered();

private:
    QAction *createAction(const QIcon& icon, const QString& name, QWidget *parent, const QList<QUrl>& urls, AdditionalJobOptions option);

    Kerfuffle::PluginManager *m_pluginManager;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ExtractFileItemAction::AdditionalJobOptions)

#endif
