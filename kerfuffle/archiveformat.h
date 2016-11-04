/*
 * Copyright (c) 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>
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
    explicit ArchiveFormat(const QMimeType& mimeType,
                           Kerfuffle::Archive::EncryptionType encryptionType,
                           int minCompLevel,
                           int maxCompLevel,
                           int defaultCompLevel,
                           bool supportsWriteComment,
                           bool supportsTesting,
                           bool suppportsMultiVolume,
                           QVariantMap compressionMethods,
                           QString defaultCompressionMethod);

    /**
     * @return The archive format of the given @p mimeType, according to the given @p metadata.
     */
    static ArchiveFormat fromMetadata(const QMimeType& mimeType, const KPluginMetaData& metadata);

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

private:
    QMimeType m_mimeType;
    Kerfuffle::Archive::EncryptionType m_encryptionType;
    int m_minCompressionLevel;
    int m_maxCompressionLevel;
    int m_defaultCompressionLevel;
    bool m_supportsWriteComment;
    bool m_supportsTesting;
    bool m_supportsMultiVolume;
    QVariantMap m_compressionMethods;
    QString m_defaultCompressionMethod;
};

}

#endif // ARCHIVEFORMAT_H
