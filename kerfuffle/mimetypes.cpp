/*
 * Copyright (c) 2016 Ragnar Thomsen <rthomsen6@gmail.com>
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

#include "mimetypes.h"
#include "ark_debug.h"

#include <QFileInfo>
#include <QMimeDatabase>
#include <QRegularExpression>
#include <QStandardPaths>

#include <KPluginLoader>

namespace Kerfuffle
{

QMimeType determineMimeType(const QString& filename)
{
    QMimeDatabase db;

    QFileInfo fileinfo(filename);
    QString inputFile = filename;

    // #328815: since detection-by-content does not work for compressed tar archives (see below why)
    // we cannot rely on it when the archive extension is wrong; we need to validate by hand.
    if (fileinfo.completeSuffix().toLower().remove(QRegularExpression(QStringLiteral("[^a-z\\.]"))).contains(QStringLiteral("tar."))) {
        inputFile.chop(fileinfo.completeSuffix().length());
        // We remove non-alpha chars from the filename extension, but not periods.
        // If the filename is e.g. "foo.tar.gz.1", we get the "foo.tar.gz." string,
        // so we need to manually drop the last period character from it.
        QString cleanExtension = fileinfo.completeSuffix().toLower().remove(QRegularExpression(QStringLiteral("[^a-z\\.]")));
        if (cleanExtension.endsWith(QLatin1Char('.'))) {
            cleanExtension.chop(1);
        }
        inputFile += cleanExtension;
        qCDebug(ARK) << "Validated filename of compressed tar" << filename << "into filename" << inputFile;
    }

    QMimeType mimeFromExtension = db.mimeTypeForFile(inputFile, QMimeDatabase::MatchExtension);
    QMimeType mimeFromContent = db.mimeTypeForFile(filename, QMimeDatabase::MatchContent);

    // mimeFromContent will be "application/octet-stream" when file is
    // unreadable, so use extension.
    if (!fileinfo.isReadable()) {
        return mimeFromExtension;
    }

    // Compressed tar-archives are detected as single compressed files when
    // detecting by content. The following code fixes detection of tar.gz, tar.bz2, tar.xz,
    // tar.lzo, tar.lz and tar.lrz.
    if ((mimeFromExtension == db.mimeTypeForName(QStringLiteral("application/x-compressed-tar")) &&
         mimeFromContent == db.mimeTypeForName(QStringLiteral("application/gzip"))) ||
        (mimeFromExtension == db.mimeTypeForName(QStringLiteral("application/x-bzip-compressed-tar")) &&
         mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-bzip"))) ||
        (mimeFromExtension == db.mimeTypeForName(QStringLiteral("application/x-xz-compressed-tar")) &&
         mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-xz"))) ||
        (mimeFromExtension == db.mimeTypeForName(QStringLiteral("application/x-tarz")) &&
         mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-compress"))) ||
        (mimeFromExtension == db.mimeTypeForName(QStringLiteral("application/x-tzo")) &&
         mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-lzop"))) ||
        (mimeFromExtension == db.mimeTypeForName(QStringLiteral("application/x-lzip-compressed-tar")) &&
         mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-lzip"))) ||
        (mimeFromExtension == db.mimeTypeForName(QStringLiteral("application/x-lrzip-compressed-tar")) &&
         mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-lrzip")))) {
        return mimeFromExtension;
    }

    if (mimeFromExtension != mimeFromContent) {

        if (mimeFromContent.isDefault()) {
            qCWarning(ARK) << "Could not detect mimetype from content."
                           << "Using extension-based mimetype:" << mimeFromExtension.name();
            return mimeFromExtension;
        }

        // #354344: ISO files are currently wrongly detected-by-content.
        if (mimeFromExtension.inherits(QStringLiteral("application/x-cd-image"))) {
            return mimeFromExtension;
        }

        qCWarning(ARK) << "Mimetype for filename extension (" << mimeFromExtension.name()
                       << ") did not match mimetype for content (" << mimeFromContent.name()
                       << "). Using content-based mimetype.";
    }

    return mimeFromContent;
}

QStringList supportedMimeTypes()
{
    const QVector<KPluginMetaData> offers = KPluginLoader::findPlugins(QStringLiteral("kerfuffle"), [](const KPluginMetaData& metaData) {
        return metaData.serviceTypes().contains(QStringLiteral("Kerfuffle/Plugin"));
    });

    QSet<QString> supported;
    foreach (const KPluginMetaData& pluginMetadata, offers) {

        const auto executables = pluginMetadata.rawData()[QStringLiteral("X-KDE-Kerfuffle-ReadOnlyExecutables")].toArray();
        if (!findExecutables(executables)) {
            qCDebug(ARK) << "Could not find all the read-only executables of" << pluginMetadata.pluginId() << "- ignoring mimetypes:" << pluginMetadata.mimeTypes();
            continue;
        }

        supported += pluginMetadata.mimeTypes().toSet();
    }

    // Remove entry for lrzipped tar if lrzip executable not found in path.
    if (QStandardPaths::findExecutable(QStringLiteral("lrzip")).isEmpty()) {
        supported.remove(QStringLiteral("application/x-lrzip-compressed-tar"));
    }

    qCDebug(ARK) << "Returning supported mimetypes" << supported;

    return sortByComment(supported);
}

QStringList supportedWriteMimeTypes()
{
    const QVector<KPluginMetaData> offers = KPluginLoader::findPlugins(QStringLiteral("kerfuffle"), [](const KPluginMetaData& metaData) {
        return metaData.serviceTypes().contains(QStringLiteral("Kerfuffle/Plugin")) &&
               metaData.rawData()[QStringLiteral("X-KDE-Kerfuffle-ReadWrite")].toBool();
    });

    QSet<QString> supported;
    foreach (const KPluginMetaData& pluginMetadata, offers) {

        const auto executables = pluginMetadata.rawData()[QStringLiteral("X-KDE-Kerfuffle-ReadWriteExecutables")].toArray();
        if (!findExecutables(executables)) {
            qCDebug(ARK) << "Could not find all the read-write executables of" << pluginMetadata.pluginId() << "- ignoring mimetypes:" << pluginMetadata.mimeTypes();
            continue;
        }

        supported += pluginMetadata.mimeTypes().toSet();
    }

    // Remove entry for lrzipped tar if lrzip executable not found in path.
    if (QStandardPaths::findExecutable(QStringLiteral("lrzip")).isEmpty()) {
        supported.remove(QStringLiteral("application/x-lrzip-compressed-tar"));
    }

    qCDebug(ARK) << "Returning supported write mimetypes" << supported;

    return sortByComment(supported);
}

QVector<KPluginMetaData> supportedWritePlugins()
{
    const auto readWritePlugins = KPluginLoader::findPlugins(QStringLiteral("kerfuffle"), [](const KPluginMetaData& metaData) {
        return metaData.serviceTypes().contains(QStringLiteral("Kerfuffle/Plugin")) &&
               metaData.rawData()[QStringLiteral("X-KDE-Kerfuffle-ReadWrite")].toBool();
    });

    // Drop plugins whose executables are not available.
    QVector<KPluginMetaData> supportedPlugins;
    foreach (const auto& plugin, readWritePlugins) {
        const QJsonArray rwExecutables = plugin.rawData()[QStringLiteral("X-KDE-Kerfuffle-ReadWriteExecutables")].toArray();
        // Libarchive does not depend on executables.
        if (rwExecutables.isEmpty()) {
            supportedPlugins << plugin;
            continue;
        }

        if (findExecutables(rwExecutables)) {
            supportedPlugins << plugin;
        }
    }

    return supportedPlugins;
}

KPluginMetaData preferredPluginFor(const QMimeType &mimeType, const QVector<KPluginMetaData> &plugins)
{
    QVector<KPluginMetaData> filteredPlugins;
    foreach (const KPluginMetaData& plugin, plugins) {
        if (plugin.mimeTypes().contains(mimeType.name())) {
            filteredPlugins << plugin;
        }
    }

    qSort(filteredPlugins.begin(), filteredPlugins.end(), comparePlugins);
    qCDebug(ARK) << "Preferred plugin for" << mimeType.name() << "is" << filteredPlugins.first().pluginId();

    return filteredPlugins.first();
}

QStringList sortByComment(const QSet<QString> &mimeTypeSet)
{
    QMap<QString,QString> map;

    // Convert QStringList to QMap to sort by comment.
    foreach (const QString &mimeType, mimeTypeSet) {
        QMimeType mime(QMimeDatabase().mimeTypeForName(mimeType));
        map[mime.comment().toLower()] = mime.name();
    }

    // Convert back to QStringList.
    QStringList mimetypeList;
    foreach (const QString &value, map) {
        mimetypeList.append(value);
    }

    return mimetypeList;
}

bool findExecutables(const QJsonArray& executables)
{
    foreach (const auto& value, executables) {
        const QString executable = value.toString();
        if (executable.isEmpty()) {
            continue;
        }

        if (QStandardPaths::findExecutable(executable).isEmpty()) {
            qCDebug(ARK) << "Could not find executable" << executable;
            return false;
        }
    }

    return true;
}

bool comparePlugins(const KPluginMetaData& p1, const KPluginMetaData& p2)
{
    return (p1.rawData()[QStringLiteral("X-KDE-Priority")].toInt()) > (p2.rawData()[QStringLiteral("X-KDE-Priority")].toInt());
}

} // namespace Kerfuffle
