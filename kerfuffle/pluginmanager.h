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
