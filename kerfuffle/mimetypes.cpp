/*
    SPDX-FileCopyrightText: 2016 Ragnar Thomsen <rthomsen6@gmail.com>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "mimetypes.h"
#include "ark_debug.h"

#include <QFileInfo>
#include <QMimeDatabase>
#include <QRegularExpression>
#include <QStandardPaths>

namespace Kerfuffle
{

QMimeType determineMimeType(const QString& filename)
{
    QMimeDatabase db;

    QFileInfo fileinfo(filename);
    QString inputFile = filename;

    // #328815: since detection-by-content does not work for compressed tar archives (see below why)
    // we cannot rely on it when the archive extension is wrong; we need to validate by hand.
    if (fileinfo.completeSuffix().toLower().remove(QRegularExpression(QStringLiteral("[^a-z\\.]"))).contains(QLatin1String("tar."))) {
        inputFile.chop(fileinfo.completeSuffix().length());
        QString cleanExtension(fileinfo.completeSuffix().toLower());

        // tar.bz2 and tar.lz4 need special treatment since they contain numbers.
        bool isBZ2 = false;
        bool isLZ4 = false;
        if (fileinfo.completeSuffix().contains(QLatin1String("bz2"), Qt::CaseInsensitive)) {
            cleanExtension.remove(QStringLiteral("bz2"));
            isBZ2 = true;
        }
        if (fileinfo.completeSuffix().contains(QLatin1String("lz4"), Qt::CaseInsensitive)) {
            cleanExtension.remove(QStringLiteral("lz4"));
            isLZ4 = true;
        }

        // We remove non-alpha chars from the filename extension, but not periods.
        // If the filename is e.g. "foo.tar.gz.1", we get the "foo.tar.gz." string,
        // so we need to manually drop the last period character from it.
        cleanExtension.remove(QRegularExpression(QStringLiteral("[^a-z\\.]")));
        if (cleanExtension.endsWith(QLatin1Char('.'))) {
            cleanExtension.chop(1);
        }

        // Re-add extension for tar.bz2 and tar.lz4.
        if (isBZ2) {
            cleanExtension.append(QStringLiteral(".bz2"));
        }
        if (isLZ4) {
            cleanExtension.append(QStringLiteral(".lz4"));
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
    // tar.lzo, tar.lz, tar.lrz and tar.zst.
    if ((mimeFromExtension.inherits(QStringLiteral("application/x-compressed-tar")) &&
         mimeFromContent == db.mimeTypeForName(QStringLiteral("application/gzip"))) ||
        (mimeFromExtension.inherits(QStringLiteral("application/x-bzip-compressed-tar")) &&
         mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-bzip"))) ||
        (mimeFromExtension.inherits(QStringLiteral("application/x-xz-compressed-tar")) &&
         mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-xz"))) ||
        (mimeFromExtension.inherits(QStringLiteral("application/x-tarz")) &&
         mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-compress"))) ||
        (mimeFromExtension.inherits(QStringLiteral("application/x-tzo")) &&
         mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-lzop"))) ||
        (mimeFromExtension.inherits(QStringLiteral("application/x-lzip-compressed-tar")) &&
         mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-lzip"))) ||
        (mimeFromExtension.inherits(QStringLiteral("application/x-lrzip-compressed-tar")) &&
         mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-lrzip"))) ||
        (mimeFromExtension.inherits(QStringLiteral("application/x-lz4-compressed-tar")) &&
         mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-lz4"))) ||
        (mimeFromExtension.inherits(QStringLiteral("application/x-zstd-compressed-tar")) &&
         mimeFromContent == db.mimeTypeForName(QStringLiteral("application/zstd")))) {
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

        qCDebug(ARK) << "Mimetype for filename extension (" << mimeFromExtension.name()
                       << ") did not match mimetype for content (" << mimeFromContent.name()
                       << "). Using content-based mimetype.";
    }

    return mimeFromContent;
}

} // namespace Kerfuffle
