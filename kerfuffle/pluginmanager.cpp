/*
    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "pluginmanager.h"
#include "ark_debug.h"
#include "settings.h"

#include <KSharedConfig>

#include <QFileInfo>
#include <QMimeDatabase>
#include <QPluginLoader>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>

#include <algorithm>

namespace Kerfuffle
{
PluginManager::PluginManager(QObject *parent)
    : QObject(parent)
{
    loadPlugins();
}

Plugin *PluginManager::pluginById(const QString &pluginId) const
{
    const auto plugins = availablePlugins();
    for (Plugin *plugin : plugins) {
        if (plugin->metaData().pluginId() == pluginId) {
            return plugin;
        }
    }
    return nullptr;
}

QList<Plugin *> PluginManager::installedPlugins() const
{
    return m_plugins;
}

QList<Plugin *> PluginManager::availablePlugins() const
{
    QList<Plugin *> availablePlugins;
    for (Plugin *plugin : std::as_const(m_plugins)) {
        if (plugin->isValid()) {
            availablePlugins << plugin;
        }
    }

    return availablePlugins;
}

QList<Plugin *> PluginManager::availableWritePlugins() const
{
    QList<Plugin *> availableWritePlugins;
    const auto plugins = availablePlugins();
    for (Plugin *plugin : plugins) {
        if (plugin->isReadWrite()) {
            availableWritePlugins << plugin;
        }
    }

    return availableWritePlugins;
}

QList<Plugin *> PluginManager::enabledPlugins() const
{
    QList<Plugin *> enabledPlugins;
    for (Plugin *plugin : std::as_const(m_plugins)) {
        if (plugin->isEnabled()) {
            enabledPlugins << plugin;
        }
    }

    return enabledPlugins;
}

QList<Plugin *> PluginManager::preferredPluginsFor(const QMimeType &mimeType)
{
    const auto mimeName = mimeType.name();
    if (m_preferredPluginsCache.contains(mimeName)) {
        return m_preferredPluginsCache.value(mimeName);
    }

    const auto plugins = preferredPluginsFor(mimeType, false);
    m_preferredPluginsCache.insert(mimeName, plugins);
    return plugins;
}

QList<Plugin *> PluginManager::preferredWritePluginsFor(const QMimeType &mimeType) const
{
    return preferredPluginsFor(mimeType, true);
}

Plugin *PluginManager::preferredPluginFor(const QMimeType &mimeType)
{
    const QList<Plugin *> preferredPlugins = preferredPluginsFor(mimeType);
    return preferredPlugins.isEmpty() ? new Plugin() : preferredPlugins.first();
}

Plugin *PluginManager::preferredWritePluginFor(const QMimeType &mimeType) const
{
    const QList<Plugin *> preferredWritePlugins = preferredWritePluginsFor(mimeType);
    return preferredWritePlugins.isEmpty() ? new Plugin() : preferredWritePlugins.first();
}

QStringList PluginManager::supportedMimeTypes(MimeSortingMode mode) const
{
    QSet<QString> supported;
    QMimeDatabase db;
    const auto plugins = availablePlugins();
    for (Plugin *plugin : plugins) {
        const auto mimeTypes = plugin->metaData().mimeTypes();
        for (const auto &mimeType : mimeTypes) {
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

    static bool s_libarchiveHasLzo = libarchiveHasLzo();
    // Remove entry for lzo-compressed tar if libarchive not linked against lzo and lzop executable not found in path.
    if (!s_libarchiveHasLzo && QStandardPaths::findExecutable(QStringLiteral("lzop")).isEmpty()) {
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
        for (const auto &mimeType : mimeTypes) {
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

    // shared-mime-info 2.3 explicitly separated application/x-bzip2-compressed-tar from application/x-bzip-compressed-tar
    // since bzip2 is not compatible with the old (and deprecated) bzip format.
    // See https://gitlab.freedesktop.org/xdg/shared-mime-info/-/merge_requests/239
    // With shared-mime-info 2.3 (or newer) we can't have both mimetypes at the same time, since libarchive does not support
    // the old deprecated bzip format. Also we can't know which version of shared-mime-info the system is actually using.
    // For these reasons, just take the mimetype from QMimeDatabase to keep the compatibility with any shared-mime-info version.
    if (supported.contains(QLatin1String("application/x-bzip-compressed-tar")) && supported.contains(QLatin1String("application/x-bzip2-compressed-tar"))) {
        supported.remove(QLatin1String("application/x-bzip-compressed-tar"));
        supported.remove(QLatin1String("application/x-bzip2-compressed-tar"));
        supported.insert(QMimeDatabase().mimeTypeForFile(QStringLiteral("dummy.tar.bz2"), QMimeDatabase::MatchExtension).name());
    }

    if (mode == SortByComment) {
        return sortByComment(supported);
    }

    return supported.values();
}

QList<Plugin *> PluginManager::filterBy(const QList<Plugin *> &plugins, const QMimeType &mimeType) const
{
    const bool supportedMime = supportedMimeTypes().contains(mimeType.name());
    QList<Plugin *> filteredPlugins;
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
    const QList<KPluginMetaData> plugins = KPluginMetaData::findPlugins(QStringLiteral("kerfuffle"));
    for (const KPluginMetaData &metaData : plugins) {
        Plugin *plugin = new Plugin(this, metaData);
        plugin->setEnabled(!ArkSettings::disabledPlugins().contains(metaData.pluginId()));
        m_plugins << plugin;
    }
}

QList<Plugin *> PluginManager::preferredPluginsFor(const QMimeType &mimeType, bool readWrite) const
{
    QList<Plugin *> preferredPlugins = filterBy((readWrite ? availableWritePlugins() : availablePlugins()), mimeType);

    std::sort(preferredPlugins.begin(), preferredPlugins.end(), [](Plugin *p1, Plugin *p2) {
        return p1->priority() > p2->priority();
    });

    return preferredPlugins;
}

QStringList PluginManager::sortByComment(const QSet<QString> &mimeTypes)
{
    QMap<QString, QString> map;

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
    const QString pluginPath = QPluginLoader(QStringLiteral("kerfuffle/kerfuffle_libarchive")).fileName();

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
    static const QRegularExpression regex(QStringLiteral("/.*/libarchive.so|/.*/libarchive.*.dylib"));
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

#include "moc_pluginmanager.cpp"
