/*
    SPDX-FileCopyrightText: 2016 Ragnar Thomsen <rthomsen6@gmail.com>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "mimetypes.h"
#include "ark_debug.h"
#include "pluginmanager.h"

#include <QFileInfo>
#include <QMimeDatabase>
#include <QRegularExpression>
#include <QStandardPaths>

namespace Kerfuffle
{
QMimeType determineMimeType(const QString &filename, MimePreference mp)
{
    QMimeDatabase db;

    QFileInfo fileinfo(filename);

    // #328815: since detection-by-content does not work for compressed tar archives (see below why)
    // we cannot rely on it when the archive extension is wrong; we need to validate by hand:
    // we look for the best match between the filename's complete suffix and all the suffixes
    // of the mimetypes supported by the libarchive plugin.
    QString validatedFilename = filename;
    static QRegularExpression nonAlphaRegex(QStringLiteral("[^a-z\\.]"));
    const QString originalSuffix(fileinfo.completeSuffix().toLower());
    QString strippedSuffix = originalSuffix;
    strippedSuffix.remove(nonAlphaRegex);
    if (strippedSuffix.contains(QLatin1String("tar."))) {
        Kerfuffle::PluginManager pluginManager;
        const Plugin *libarchivePlugin = pluginManager.pluginById(QLatin1String("kerfuffle_libarchive"));
        if (libarchivePlugin) {
            QString bestSuffixMatch;
            const QStringList libarchiveMimeTypes = libarchivePlugin->metaData().mimeTypes();
            for (const QString &mimeName : libarchiveMimeTypes) {
                const QStringList mimeSuffixes = db.mimeTypeForName(mimeName).suffixes();
                for (const QString &suffix : mimeSuffixes) {
                    const QString candidateSuffix = suffix.toLower();
                    QString tempSuffixMatch;
                    if (originalSuffix.contains(candidateSuffix)) {
                        tempSuffixMatch = candidateSuffix;
                    } else if (strippedSuffix.contains(candidateSuffix)) {
                        tempSuffixMatch = candidateSuffix;
                    }
                    // We are only interested in the longest match
                    // (e.g. foo.tar.lz4 matches 'tar', 'tar.lz' and 'tar.lz4' suffixes)
                    if (tempSuffixMatch.length() > bestSuffixMatch.length()) {
                        bestSuffixMatch = tempSuffixMatch;
                    }
                }
            }
            if (!bestSuffixMatch.isEmpty()) {
                validatedFilename.chop(originalSuffix.length());
                validatedFilename += bestSuffixMatch;
                qCDebug(ARK_LOG) << "Validated filename of compressed tar" << filename << "into filename" << validatedFilename;
            }
        }
    }

    QMimeType mimeFromExtension = db.mimeTypeForFile(validatedFilename, QMimeDatabase::MatchExtension);
    QMimeType mimeFromContent = db.mimeTypeForFile(filename, QMimeDatabase::MatchContent);

    // mimeFromContent will be "application/octet-stream" when file is
    // unreadable, so use extension.
    if (!fileinfo.isReadable()) {
        return mimeFromExtension;
    }

    // Compressed tar-archives are detected as single compressed files when
    // detecting by content. The following code fixes detection of tar.gz, tar.bz2, tar.xz,
    // tar.lzo, tar.lz, tar.lrz and tar.zst.
    if ((mimeFromExtension.inherits(QStringLiteral("application/x-compressed-tar"))
         && mimeFromContent == db.mimeTypeForName(QStringLiteral("application/gzip")))
        || (mimeFromExtension.inherits(QStringLiteral("application/x-bzip-compressed-tar"))
            && mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-bzip")))
        || (mimeFromExtension.inherits(QStringLiteral("application/x-bzip2-compressed-tar"))
            && mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-bzip2")))
        || (mimeFromExtension.inherits(QStringLiteral("application/x-xz-compressed-tar"))
            && mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-xz")))
        || (mimeFromExtension.inherits(QStringLiteral("application/x-tarz")) && mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-compress")))
        || (mimeFromExtension.inherits(QStringLiteral("application/x-tzo")) && mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-lzop")))
        || (mimeFromExtension.inherits(QStringLiteral("application/x-lzip-compressed-tar"))
            && mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-lzip")))
        || (mimeFromExtension.inherits(QStringLiteral("application/x-lrzip-compressed-tar"))
            && mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-lrzip")))
        || (mimeFromExtension.inherits(QStringLiteral("application/x-lz4-compressed-tar"))
            && mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-lz4")))
        || (mimeFromExtension.inherits(QStringLiteral("application/x-zstd-compressed-tar"))
            && mimeFromContent == db.mimeTypeForName(QStringLiteral("application/zstd")))) {
        return mimeFromExtension;
    }

    if (mimeFromExtension != mimeFromContent) {
        if (mimeFromContent.isDefault()) {
            qCWarning(ARK_LOG) << "Could not detect mimetype from content."
                               << "Using extension-based mimetype:" << mimeFromExtension.name();
            return mimeFromExtension;
        }

        qCDebug(ARK_LOG) << "Mimetype for filename extension (" << mimeFromExtension.name() << ") did not match mimetype for content ("
                         << mimeFromContent.name() << "). Using content-based mimetype.";
    }

    return mp == PreferExtensionMime ? mimeFromExtension : mimeFromContent;
}

} // namespace Kerfuffle
