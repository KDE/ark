/*
    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef ARCHIVEFORMAT_H
#define ARCHIVEFORMAT_H

#include "archive_kerfuffle.h"

#include <KPluginMetaData>

namespace Kerfuffle
{
class KERFUFFLE_EXPORT ArchiveFormat
{
public:
    explicit ArchiveFormat();
    explicit ArchiveFormat(const QMimeType &mimeType,
                           Kerfuffle::Archive::EncryptionType encryptionType,
                           int minCompLevel,
                           int maxCompLevel,
                           int defaultCompLevel,
                           bool supportsWriteComment,
                           bool supportsTesting,
                           bool suppportsMultiVolume,
                           const QVariantMap &compressionMethods,
                           const QString &defaultCompressionMethod,
                           const QStringList &encryptionMethods,
                           const QString &defaultEncryptionMethod);

    /**
     * @return The archive format of the given @p mimeType, according to the given @p metadata.
     */
    static ArchiveFormat fromMetadata(const QMimeType &mimeType, const KPluginMetaData &metadata);

    /**
     * @return Whether the format is associated to a valid mimetype.
     */
    bool isValid() const;

    /**
     * @return The encryption type supported by the archive format.
     */
    Kerfuffle::Archive::EncryptionType encryptionType() const;

    int minCompressionLevel() const;
    int maxCompressionLevel() const;
    int defaultCompressionLevel() const;
    bool supportsWriteComment() const;
    bool supportsTesting() const;
    bool supportsMultiVolume() const;
    QVariantMap compressionMethods() const;
    QString defaultCompressionMethod() const;
    QStringList encryptionMethods() const;
    QString defaultEncryptionMethod() const;

private:
    QMimeType m_mimeType;
    Kerfuffle::Archive::EncryptionType m_encryptionType = Kerfuffle::Archive::Unencrypted;
    int m_minCompressionLevel = -1;
    int m_maxCompressionLevel = 0;
    int m_defaultCompressionLevel = 0;
    bool m_supportsWriteComment = false;
    bool m_supportsTesting = false;
    bool m_supportsMultiVolume = false;
    QVariantMap m_compressionMethods;
    QString m_defaultCompressionMethod;
    QStringList m_encryptionMethods;
    QString m_defaultEncryptionMethod;
};

}

#endif // ARCHIVEFORMAT_H
