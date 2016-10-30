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

#include "pluginmanager.h"

#include <KConfigGroup>
#include <KPluginLoader>
#include <KSharedConfig>

#include <QMimeDatabase>
#include <QSet>

#include <algorithm>

namespace Kerfuffle
{

PluginManager::PluginManager(QObject *parent) : QObject(parent)
{
    loadPlugins();
}

QVector<Plugin*> PluginManager::installedPlugins() const
{
    return m_plugins;
}

QVector<Plugin*> PluginManager::availablePlugins() const
{
    QVector<Plugin*> availablePlugins;
    foreach (Plugin *plugin, m_plugins) {
        if (plugin->isValid()) {
            availablePlugins << plugin;
        }
    }

    return availablePlugins;
}

QVector<Plugin*> PluginManager::availableWritePlugins() const
{
    QVector<Plugin*> availableWritePlugins;
    foreach (Plugin *plugin, availablePlugins()) {
        if (plugin->isReadWrite()) {
            availableWritePlugins << plugin;
        }
    }

    return availableWritePlugins;
}

QVector<Plugin*> PluginManager::enabledPlugins() const
{
    QVector<Plugin*> enabledPlugins;
    foreach (Plugin *plugin, m_plugins) {
        if (plugin->isEnabled()) {
            enabledPlugins << plugin;
        }
    }

    return enabledPlugins;
}

QVector<Plugin*> PluginManager::preferredPluginsFor(const QMimeType &mimeType) const
{
    return preferredPluginsFor(mimeType, false);
}

QVector<Plugin*> PluginManager::preferredWritePluginsFor(const QMimeType &mimeType) const
{
    return preferredPluginsFor(mimeType, true);
}

Plugin *PluginManager::preferredPluginFor(const QMimeType &mimeType) const
{
    const QVector<Plugin*> preferredPlugins = preferredPluginsFor(mimeType);
    return preferredPlugins.isEmpty() ? new Plugin() : preferredPlugins.first();
}

Plugin *PluginManager::preferredWritePluginFor(const QMimeType &mimeType) const
{
    const QVector<Plugin*> preferredWritePlugins = preferredWritePluginsFor(mimeType);
    return preferredWritePlugins.isEmpty() ? new Plugin() : preferredWritePlugins.first();
}

QStringList PluginManager::supportedMimeTypes() const
{
    QSet<QString> supported;
    foreach (Plugin *plugin, availablePlugins()) {
        supported += plugin->metaData().mimeTypes().toSet();
    }

    // Remove entry for lrzipped tar if lrzip executable not found in path.
    if (QStandardPaths::findExecutable(QStringLiteral("lrzip")).isEmpty()) {
        supported.remove(QStringLiteral("application/x-lrzip-compressed-tar"));
    }

    // Remove entry for lz4-compressed tar if lz4 executable not found in path.
    if (QStandardPaths::findExecutable(QStringLiteral("lz4")).isEmpty()) {
        supported.remove(QStringLiteral("application/x-lz4-compressed-tar"));
    }

    return sortByComment(supported);
}

QStringList PluginManager::supportedWriteMimeTypes() const
{
    QSet<QString> supported;
    foreach (Plugin *plugin, availableWritePlugins()) {
        supported += plugin->metaData().mimeTypes().toSet();
    }

    // Remove entry for lrzipped tar if lrzip executable not found in path.
    if (QStandardPaths::findExecutable(QStringLiteral("lrzip")).isEmpty()) {
        supported.remove(QStringLiteral("application/x-lrzip-compressed-tar"));
    }

    // Remove entry for lz4-compressed tar if lz4 executable not found in path.
    if (QStandardPaths::findExecutable(QStringLiteral("lz4")).isEmpty()) {
        supported.remove(QStringLiteral("application/x-lz4-compressed-tar"));
    }

    return sortByComment(supported);
}

QVector<Plugin*> PluginManager::filterBy(const QVector<Plugin*> &plugins, const QMimeType &mimeType) const
{
    const bool supportedMime = supportedMimeTypes().contains(mimeType.name());
    QVector<Plugin*> filteredPlugins;
    foreach (Plugin *plugin, plugins) {
        if (!supportedMime) {
            // Check whether the mimetype inherits from a supported mimetype.
            foreach (const QString &mime, plugin->metaData().mimeTypes()) {
                if (mimeType.inherits(mime)) {
                    filteredPlugins << plugin;
                }
            }
        } else if (plugin->metaData().mimeTypes().contains(mimeType.name())) {
            filteredPlugins << plugin;
        }
    }

    return filteredPlugins;
}

void PluginManager::loadPlugins()
{
    const QVector<KPluginMetaData> plugins = KPluginLoader::findPlugins(QStringLiteral("kerfuffle"));
    // This class might be used from executables other than ark (e.g. the tests),
    // so we need to specify the name of the config file.
    // TODO: once we have a GUI in the settings dialog,
    // use this group to write whether a plugin gets disabled.
    const KConfigGroup conf(KSharedConfig::openConfig(QStringLiteral("arkrc")), "EnabledPlugins");

    QSet<QString> addedPlugins;
    foreach (const KPluginMetaData &metaData, plugins) {
        const auto pluginId = metaData.pluginId();
        // Filter out duplicate plugins.
        if (addedPlugins.contains(pluginId)) {
            continue;
        }

        Plugin *plugin = new Plugin(this, metaData);
        plugin->setEnabled(conf.readEntry(pluginId, true));
        addedPlugins << pluginId;
        m_plugins << plugin;
    }
}

QVector<Plugin*> PluginManager::preferredPluginsFor(const QMimeType &mimeType, bool readWrite) const
{
    QVector<Plugin*> preferredPlugins = filterBy((readWrite ? availableWritePlugins() : availablePlugins()), mimeType);

    std::sort(preferredPlugins.begin(), preferredPlugins.end(), [](Plugin* p1, Plugin* p2) {
        return p1->priority() > p2->priority();
    });

    return preferredPlugins;
}

QStringList PluginManager::sortByComment(const QSet<QString> &mimeTypes)
{
    QMap<QString,QString> map;

    // Initialize the QMap to sort by comment.
    foreach (const QString &mimeType, mimeTypes) {
        QMimeType mime(QMimeDatabase().mimeTypeForName(mimeType));
        if (mime.isValid()) {
            map[mime.comment().toLower()] = mime.name();
        }
    }

    // Convert to sorted QStringList.
    QStringList sortedMimeTypes;
    foreach (const QString &value, map) {
        sortedMimeTypes << value;
    }

    return sortedMimeTypes;
}

}
