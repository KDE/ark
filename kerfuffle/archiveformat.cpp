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

#include "archiveformat.h"
#include "ark_debug.h"

#include <QJsonArray>

namespace Kerfuffle
{

ArchiveFormat::ArchiveFormat() :
    m_encryptionType(Archive::Unencrypted)
{
}

ArchiveFormat::ArchiveFormat(const QMimeType& mimeType,
                             Archive::EncryptionType encryptionType,
                             int minCompLevel,
                             int maxCompLevel,
                             int defaultCompLevel,
                             bool supportsWriteComment,
                             bool supportsTesting,
                             bool supportsMultiVolume,
                             QVariantMap compressionMethods,
                             QString defaultCompressionMethod) :
    m_mimeType(mimeType),
    m_encryptionType(encryptionType),
    m_minCompressionLevel(minCompLevel),
    m_maxCompressionLevel(maxCompLevel),
    m_defaultCompressionLevel(defaultCompLevel),
    m_supportsWriteComment(supportsWriteComment),
    m_supportsTesting(supportsTesting),
    m_supportsMultiVolume(supportsMultiVolume),
    m_compressionMethods(compressionMethods),
    m_defaultCompressionMethod(defaultCompressionMethod)
{
}

ArchiveFormat ArchiveFormat::fromMetadata(const QMimeType& mimeType, const KPluginMetaData& metadata)
{
    const QJsonObject json = metadata.rawData();
    foreach (const QString& mime, metadata.mimeTypes()) {
        if (mimeType.name() != mime) {
            continue;
        }

        const QJsonObject formatProps = json[mime].toObject();

        int minCompLevel = formatProps[QStringLiteral("CompressionLevelMin")].toInt();
        int maxCompLevel = formatProps[QStringLiteral("CompressionLevelMax")].toInt();
        int defaultCompLevel = formatProps[QStringLiteral("CompressionLevelDefault")].toInt();

        bool supportsWriteComment = formatProps[QStringLiteral("SupportsWriteComment")].toBool();
        bool supportsTesting = formatProps[QStringLiteral("SupportsTesting")].toBool();
        bool supportsMultiVolume = formatProps[QStringLiteral("SupportsMultiVolume")].toBool();

        QVariantMap compressionMethods = formatProps[QStringLiteral("CompressionMethods")].toObject().toVariantMap();
        QString defaultCompMethod = formatProps[QStringLiteral("CompressionMethodDefault")].toString();

        Archive::EncryptionType encType = Archive::Unencrypted;
        if (formatProps[QStringLiteral("HeaderEncryption")].toBool()) {
            encType = Archive::HeaderEncrypted;
        } else if (formatProps[QStringLiteral("Encryption")].toBool()) {
            encType = Archive::Encrypted;
        }

        return ArchiveFormat(mimeType, encType, minCompLevel, maxCompLevel, defaultCompLevel, supportsWriteComment, supportsTesting, supportsMultiVolume, compressionMethods, defaultCompMethod);
    }

    return ArchiveFormat();
}

bool ArchiveFormat::isValid() const
{
    return m_mimeType.isValid();
}

Archive::EncryptionType ArchiveFormat::encryptionType() const
{
    return m_encryptionType;
}

int ArchiveFormat::minCompressionLevel() const
{
    return m_minCompressionLevel;
}

int ArchiveFormat::maxCompressionLevel() const
{
    return m_maxCompressionLevel;
}

int ArchiveFormat::defaultCompressionLevel() const
{
    return m_defaultCompressionLevel;
}

bool ArchiveFormat::supportsWriteComment() const
{
    return m_supportsWriteComment;
}

bool ArchiveFormat::supportsTesting() const
{
    return m_supportsTesting;
}

bool ArchiveFormat::supportsMultiVolume() const
{
    return m_supportsMultiVolume;
}

QVariantMap ArchiveFormat::compressionMethods() const
{
    return m_compressionMethods;
}

QString ArchiveFormat::defaultCompressionMethod() const
{
    return m_defaultCompressionMethod;
}

}
