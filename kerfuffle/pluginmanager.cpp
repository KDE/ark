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
#include "ark_debug.h"
#include "settings.h"

#include <KPluginLoader>
#include <KSharedConfig>

#include <QFileInfo>
#include <QMimeDatabase>
#include <QProcess>
#include <QRegularExpression>
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
    for (Plugin *plugin : std::as_const(m_plugins)) {
        if (plugin->isValid()) {
            availablePlugins << plugin;
        }
    }

    return availablePlugins;
}

QVector<Plugin*> PluginManager::availableWritePlugins() const
{
    QVector<Plugin*> availableWritePlugins;
    const auto plugins = availablePlugins();
    for (Plugin *plugin : plugins) {
        if (plugin->isReadWrite()) {
            availableWritePlugins << plugin;
        }
    }

    return availableWritePlugins;
}

QVector<Plugin*> PluginManager::enabledPlugins() const
{
    QVector<Plugin*> enabledPlugins;
    for (Plugin *plugin : std::as_const(m_plugins)) {
        if (plugin->isEnabled()) {
            enabledPlugins << plugin;
        }
    }

    return enabledPlugins;
}

QVector<Plugin*> PluginManager::preferredPluginsFor(const QMimeType &mimeType)
{
    const auto mimeName = mimeType.name();
    if (m_preferredPluginsCache.contains(mimeName)) {
        return m_preferredPluginsCache.value(mimeName);
    }

    const auto plugins = preferredPluginsFor(mimeType, false);
    m_preferredPluginsCache.insert(mimeName, plugins);
    return plugins;
}

QVector<Plugin*> PluginManager::preferredWritePluginsFor(const QMimeType &mimeType) const
{
    return preferredPluginsFor(mimeType, true);
}

Plugin *PluginManager::preferredPluginFor(const QMimeType &mimeType)
{
    const QVector<Plugin*> preferredPlugins = preferredPluginsFor(mimeType);
    return preferredPlugins.isEmpty() ? new Plugin() : preferredPlugins.first();
}

Plugin *PluginManager::preferredWritePluginFor(const QMimeType &mimeType) const
{
    const QVector<Plugin*> preferredWritePlugins = preferredWritePluginsFor(mimeType);
    return preferredWritePlugins.isEmpty() ? new Plugin() : preferredWritePlugins.first();
}

QStringList PluginManager::supportedMimeTypes(MimeSortingMode mode) const
{
    QSet<QString> supported;
    QMimeDatabase db;
    const auto plugins = availablePlugins();
    for (Plugin *plugin : plugins) {
        const auto mimeTypes = plugin->metaData().mimeTypes();
        for (const auto& mimeType : mimeTypes) {
            if (db.mimeTypeForName(mimeType).isValid()) {
                supported.insert(mimeType);
            }
        }
    }

    // Remove entry for lrzipped tar if lrzip executable not found in path.
    if (QStandardPaths::findExecutable(QStringLiteral("lrzip")).isEmpty()) {
        supported.remove(QStringLiteral("application/x-lrzip-compressed-tar"));
    }

    // Remove entry for lz4-compressed tar if lz4 executable not found in path.
    if (QStandardPaths::findExecutable(QStringLiteral("lz4")).isEmpty()) {
        supported.remove(QStringLiteral("application/x-lz4-compressed-tar"));
    }

    // Remove entry for lzo-compressed tar if libarchive not linked against lzo and lzop executable not found in path.
    if (!libarchiveHasLzo() && QStandardPaths::findExecutable(QStringLiteral("lzop")).isEmpty()) {
        supported.remove(QStringLiteral("application/x-tzo"));
    }

    if (mode == SortByComment) {
        return sortByComment(supported);
    }

    return supported.values();
}

QStringList PluginManager::supportedWriteMimeTypes(MimeSortingMode mode) const
{
    QSet<QString> supported;
    QMimeDatabase db;
    const auto plugins = availableWritePlugins();
    for (Plugin *plugin : plugins) {
        const auto mimeTypes = plugin->metaData().mimeTypes();
        for (const auto& mimeType : mimeTypes) {
            if (db.mimeTypeForName(mimeType).isValid()) {
                supported.insert(mimeType);
            }
        }
    }

    // Remove entry for lrzipped tar if lrzip executable not found in path.
    if (QStandardPaths::findExecutable(QStringLiteral("lrzip")).isEmpty()) {
        supported.remove(QStringLiteral("application/x-lrzip-compressed-tar"));
    }

    // Remove entry for lz4-compressed tar if lz4 executable not found in path.
    if (QStandardPaths::findExecutable(QStringLiteral("lz4")).isEmpty()) {
        supported.remove(QStringLiteral("application/x-lz4-compressed-tar"));
    }

    // Remove entry for lzo-compressed tar if libarchive not linked against lzo and lzop executable not found in path.
    if (!libarchiveHasLzo() && QStandardPaths::findExecutable(QStringLiteral("lzop")).isEmpty()) {
        supported.remove(QStringLiteral("application/x-tzo"));
    }

    if (mode == SortByComment) {
        return sortByComment(supported);
    }

    return supported.values();
}

QVector<Plugin*> PluginManager::filterBy(const QVector<Plugin*> &plugins, const QMimeType &mimeType) const
{
    const bool supportedMime = supportedMimeTypes().contains(mimeType.name());
    QVector<Plugin*> filteredPlugins;
    for (Plugin *plugin : plugins) {
        if (!supportedMime) {
            // Check whether the mimetype inherits from a supported mimetype.
            const QStringList mimeTypes = plugin->metaData().mimeTypes();
            for (const QString &mime : mimeTypes) {
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
    QSet<QString> addedPlugins;
    for (const KPluginMetaData &metaData : plugins) {
        const auto pluginId = metaData.pluginId();
        // Filter out duplicate plugins.
        if (addedPlugins.contains(pluginId)) {
            continue;
        }

        Plugin *plugin = new Plugin(this, metaData);
        plugin->setEnabled(!ArkSettings::disabledPlugins().contains(pluginId));
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
    for (const QString &mimeType : mimeTypes) {
        QMimeType mime(QMimeDatabase().mimeTypeForName(mimeType));
        map[mime.comment().toLower()] = mime.name();
    }

    // Convert to sorted QStringList.
    QStringList sortedMimeTypes;
    for (const QString &value : std::as_const(map)) {
        sortedMimeTypes << value;
    }

    return sortedMimeTypes;
}

bool PluginManager::libarchiveHasLzo()
{
    // Step 1: look for the libarchive plugin, which is built against libarchive.
    const QString pluginPath = []() {
        const QStringList paths = QCoreApplication::libraryPaths();
        for (const QString &path : paths) {
            const QString pluginPath = QStringLiteral("%1/kerfuffle/kerfuffle_libarchive.so").arg(path);
            if (QFileInfo::exists(pluginPath)) {
                return pluginPath;
            }
        }

        return QString();
    }();

    // Step 2: process the libarchive plugin dependencies to figure out the absolute libarchive path.
    QProcess dependencyTool;
    QStringList args;
#ifdef DEPENDENCY_TOOL_ARGS
    args << QStringLiteral(DEPENDENCY_TOOL_ARGS);
#endif
    dependencyTool.setProgram(QStringLiteral(DEPENDENCY_TOOL));
    dependencyTool.setArguments(args + QStringList(pluginPath));
    dependencyTool.start();
    dependencyTool.waitForFinished();
    const QString output = QString::fromUtf8(dependencyTool.readAllStandardOutput());
    QRegularExpression regex(QStringLiteral("/.*/libarchive.so|/.*/libarchive.*.dylib"));
    if (!regex.match(output).hasMatch()) {
        return false;
    }

    // Step 3: check whether libarchive links against liblzo.
    const QStringList libarchivePath(regex.match(output).captured(0));
    dependencyTool.setArguments(args + libarchivePath);
    dependencyTool.start();
    dependencyTool.waitForFinished();
    return dependencyTool.readAllStandardOutput().contains(QByteArrayLiteral("lzo"));
}

}
