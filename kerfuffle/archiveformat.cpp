/*
 * Copyright (c) 2016 Elvis Angelaccio <elvis.angelaccio@kdemail.net>
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

namespace Kerfuffle
{

ArchiveFormat::ArchiveFormat() :
    m_encryptionType(Archive::Unencrypted)
{
}

ArchiveFormat::ArchiveFormat(const QMimeType& mimeType, Archive::EncryptionType encryptionType) :
    m_mimeType(mimeType),
    m_encryptionType(encryptionType)
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
        if (formatProps.isEmpty()) {
            return ArchiveFormat(mimeType, Archive::Unencrypted);
        }

        if (formatProps[QStringLiteral("HeaderEncryption")].toBool()) {
            return ArchiveFormat(mimeType, Archive::HeaderEncrypted);
        }

        if (formatProps[QStringLiteral("Encryption")].toBool()) {
            return ArchiveFormat(mimeType, Archive::Encrypted);
        }
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

}
