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
#include <KPluginMetaData>

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
    // tar.lzo and tar.lrz.
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

        const QStringList executables = pluginMetadata.rawData()[QStringLiteral("X-KDE-Kerfuffle-ReadOnlyExecutables")].toString().split(QLatin1Char(';'));
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

QSet<QString> supportedWriteMimeTypes()
{
    const QVector<KPluginMetaData> offers = KPluginLoader::findPlugins(QStringLiteral("kerfuffle"), [](const KPluginMetaData& metaData) {
        return metaData.serviceTypes().contains(QStringLiteral("Kerfuffle/Plugin")) &&
               metaData.rawData()[QStringLiteral("X-KDE-Kerfuffle-ReadWrite")].toVariant().toBool();
    });

    QSet<QString> supported;
    foreach (const KPluginMetaData& pluginMetadata, offers) {

        const QStringList executables = pluginMetadata.rawData()[QStringLiteral("X-KDE-Kerfuffle-ReadWriteExecutables")].toString().split(QLatin1Char(';'));
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

    return supported;
}

QSet<QString> supportedEncryptEntriesMimeTypes()
{
    const QVector<KPluginMetaData> offers = KPluginLoader::findPlugins(QStringLiteral("kerfuffle"), [](const KPluginMetaData& metaData) {
        return metaData.serviceTypes().contains(QStringLiteral("Kerfuffle/Plugin"));
    });

    QSet<QString> supported;

    foreach (const KPluginMetaData& pluginMetadata, offers) {
        const QStringList mimeTypes = pluginMetadata.rawData()[QStringLiteral("X-KDE-Kerfuffle-EncryptEntries")].toString().split(QLatin1Char(','));
        foreach (const QString& mimeType, mimeTypes) {
            supported.insert(mimeType);
        }
    }

    qCDebug(ARK) << "Entry encryption supported for mimetypes" << supported;

    return supported;
}

QSet<QString> supportedEncryptHeaderMimeTypes()
{
    const QVector<KPluginMetaData> offers = KPluginLoader::findPlugins(QStringLiteral("kerfuffle"), [](const KPluginMetaData& metaData) {
        return metaData.serviceTypes().contains(QStringLiteral("Kerfuffle/Plugin"));
    });

    QSet<QString> supported;

    foreach (const KPluginMetaData& pluginMetadata, offers) {
        const QStringList mimeTypes = pluginMetadata.rawData()[QStringLiteral("X-KDE-Kerfuffle-EncryptHeader")].toString().split(QLatin1Char(','));
        foreach (const QString& mimeType, mimeTypes) {
            supported.insert(mimeType);
        }
    }

    qCDebug(ARK) << "Header encryption supported for mimetypes" << supported;

    return supported;
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

bool findExecutables(const QStringList& executables)
{
    foreach (const QString& executable, executables) {

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

} // namespace Kerfuffle
