/*
    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef PLUGINSETTINGSPAGE_H
#define PLUGINSETTINGSPAGE_H

#include "settingspage.h"
#include "pluginmanager.h"
#include "ui_pluginsettingspage.h"

class QTreeWidgetItem;

namespace Kerfuffle
{
class KERFUFFLE_EXPORT PluginSettingsPage : public SettingsPage, public Ui::PluginSettingsPage
{
    Q_OBJECT

public:
    explicit PluginSettingsPage(QWidget *parent = nullptr, const QString &name = QString(), const QString &iconName = QString());

public Q_SLOTS:
    void slotSettingsChanged() override;
    void slotDefaultsButtonClicked() override;

private Q_SLOTS:
    void slotItemChanged(QTreeWidgetItem *item);

private:
    QStringList m_toBeDisabled;  // List of plugins that will be disabled upon clicking the Apply button.
    PluginManager m_pluginManager;
};
}

#endif
