/*
    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include "plugin.h"

#include <QMimeType>
#include <QVector>
#include <QHash>

namespace Kerfuffle
{

class KERFUFFLE_EXPORT PluginManager : public QObject
{
    Q_OBJECT

public:

    /**
     * How the list of supported mimetypes can be sorted.
     */
    enum MimeSortingMode
    {
        Unsorted,
        SortByComment
    };

    explicit PluginManager(QObject *parent = nullptr);

    /**
     * @return The list of all installed plugins.
     * An installed plugin is not necessarily available to the app.
     * The user could have disabled it from the settings, or the needed executables could not be found.
     */
    QVector<Plugin*> installedPlugins() const;

    /**
     * @return The list of plugins ready to be used. Includes read-only and read-write ones.
     */
    QVector<Plugin*> availablePlugins() const;

    /**
     * @return The list of read-write plugins ready to be used.
     */
    QVector<Plugin*> availableWritePlugins() const;

    /**
     * @return The list of plugins enabled by the user in the settings dialog.
     */
    QVector<Plugin*> enabledPlugins() const;

    /**
     * @return The list of preferred plugins for the given @p mimeType, among all the available ones.
     * The list is sorted according to the plugins priority. The list is saved in a cache for efficiency.
     * If no plugin is available, returns an empty list.
     */
    QVector<Plugin*> preferredPluginsFor(const QMimeType &mimeType);

    /**
     * @return The list of preferred read-write plugins for the given @p mimeType, among all the available ones.
     * The list is sorted according to the plugins priority.
     * If no read-write plugin is available, returns an empty list.
     */
    QVector<Plugin*> preferredWritePluginsFor(const QMimeType &mimeType) const;

    /**
     * @return The preferred plugin for the given @p mimeType, among all the available ones.
     * If no plugin is available, returns an invalid plugin.
     */
    Plugin *preferredPluginFor(const QMimeType &mimeType);

    /**
     * @return The preferred read-write plugin for the given @p mimeType, among all the available ones.
     * If no read-write plugin is available, returns an invalid plugin.
     */
    Plugin *preferredWritePluginFor(const QMimeType &mimeType) const;

    /**
     * @return The list of all mimetypes that Ark can open, sorted according to @p mode.
     */
    QStringList supportedMimeTypes(MimeSortingMode mode = Unsorted) const;

    /**
     * @return The list of all read-write mimetypes supported by Ark, sorted according to @p mode.
     */
    QStringList supportedWriteMimeTypes(MimeSortingMode mode = Unsorted) const;

    /**
     * @return The subset of @p plugins that support either @p mimetype or a parent of @p mimetype.
     */
    QVector<Plugin*> filterBy(const QVector<Plugin*> &plugins, const QMimeType &mimeType) const;

private:

    void loadPlugins();

    /**
     * @param readWrite whether to return only the read-write plugins.
     * @return The list of preferred plugins for @p mimeType among the available ones, sorted by priority.
     */
    QVector<Plugin*> preferredPluginsFor(const QMimeType &mimeType, bool readWrite) const;

    /**
     * @return A list with the given @p mimeTypes, alphabetically sorted according to their comment.
     */
    static QStringList sortByComment(const QSet<QString> &mimeTypes);

    /**
     * Workaround for libarchive >= 3.3 not linking against liblzo.
     */
    static bool libarchiveHasLzo();

    QVector<Plugin*> m_plugins;
    QHash<QString, QVector<Plugin*>> m_preferredPluginsCache;
};

}

#endif
