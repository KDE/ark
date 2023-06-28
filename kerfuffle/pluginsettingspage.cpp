/*
    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "pluginsettingspage.h"
#include "ark_debug.h"

#include <QTreeWidget>

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

#include "moc_pluginsettingspage.cpp"
