/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES ( INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION ) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * ( INCLUDING NEGLIGENCE OR OTHERWISE ) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "pluginsettingspage.h"
#include "ark_debug.h"

#include <QTreeWidget>

#include <KConfigDialogManager>

namespace Kerfuffle
{

PluginSettingsPage::PluginSettingsPage(QWidget *parent, const QString &name, const QString &iconName)
    : SettingsPage(parent, name, iconName)
{
    setupUi(this);

    const auto installedPlugins = m_pluginManager.installedPlugins();
    for (const auto plugin : installedPlugins) {
        const auto metaData = plugin->metaData();
        auto item = new QTreeWidgetItem(kcfg_disabledPlugins);
        item->setData(0, Qt::UserRole, QVariant::fromValue(plugin));
        item->setText(0, metaData.name());
        item->setText(1, metaData.description());
        item->setCheckState(0, plugin->isEnabled() ? Qt::Checked : Qt::Unchecked);
        if (!plugin->isEnabled()) {
            m_toBeDisabled << metaData.pluginId();
        }
        if (!plugin->hasRequiredExecutables()) {
            item->setDisabled(true);
            for (int i : {0, 1}) {
                item->setToolTip(i, i18n("The plugin cannot be used because one or more required executables are missing. Check your installation."));
            }
        }
    }

    for (int i : {0, 1}) {
        kcfg_disabledPlugins->resizeColumnToContents(i);
    }
    kcfg_disabledPlugins->sortItems(0, Qt::AscendingOrder);
    connect(kcfg_disabledPlugins, &QTreeWidget::itemChanged, this, &PluginSettingsPage::slotItemChanged);

    // Set the custom property that KConfigDialogManager will use to update the settings.
    kcfg_disabledPlugins->setProperty("kcfg_property", QByteArray("disabledPlugins"));
    // Set the custom property that KConfigDialogManager will use to monitor signals for changes.
    kcfg_disabledPlugins->setProperty("kcfg_propertyNotify", QByteArray(SIGNAL(itemChanged(QTreeWidgetItem*,int))));
}

void PluginSettingsPage::slotSettingsChanged()
{
    m_toBeDisabled.clear();
}

void PluginSettingsPage::slotDefaultsButtonClicked()
{
    // KConfigDialogManager doesn't know how to reset the QTreeWidget,  we need to do it manually.
    QTreeWidgetItemIterator iterator(kcfg_disabledPlugins);
    while (*iterator) {
        auto item = *iterator;
        // By default every plugin is enabled.
        item->setCheckState(0, Qt::Checked);
        iterator++;
    }
}

void PluginSettingsPage::slotItemChanged(QTreeWidgetItem *item)
{
    auto plugin = item->data(0, Qt::UserRole).value<Plugin*>();
    if (!plugin) {
        return;
    }

    const auto pluginId = plugin->metaData().pluginId();
    plugin->setEnabled(item->checkState(0) == Qt::Checked);
    // If unchecked, add to the list of plugins that will be disabled.
    m_toBeDisabled.removeAll(pluginId);
    if (!plugin->isEnabled()) {
        m_toBeDisabled << pluginId;
    }
    // Enable the Apply button by setting the property.
    qCDebug(ARK) << "Going to disable the following plugins:" << m_toBeDisabled;
    kcfg_disabledPlugins->setProperty("disabledPlugins", m_toBeDisabled);
}

}

