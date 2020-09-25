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

#include "options.h"

namespace Kerfuffle
{

bool Options::encryptedArchiveHint() const
{
    return m_encryptedArchiveHint;
}

void Options::setEncryptedArchiveHint(bool encrypted)
{
    m_encryptedArchiveHint = encrypted;
}

bool ExtractionOptions::preservePaths() const
{
    return m_preservePaths;
}

void ExtractionOptions::setPreservePaths(bool preservePaths)
{
    m_preservePaths = preservePaths;
}

bool ExtractionOptions::isDragAndDropEnabled() const
{
    return m_dragAndDrop;
}

void ExtractionOptions::setDragAndDropEnabled(bool enabled)
{
    m_dragAndDrop = enabled;
}

bool ExtractionOptions::alwaysUseTempDir() const
{
    return m_alwaysUseTempDir;
}

void ExtractionOptions::setAlwaysUseTempDir(bool alwaysUseTempDir)
{
    m_alwaysUseTempDir = alwaysUseTempDir;
}

bool CompressionOptions::isCompressionLevelSet() const
{
    return compressionLevel() != -1;
}

bool CompressionOptions::isVolumeSizeSet() const
{
    return volumeSize() > 0;
}

int CompressionOptions::compressionLevel() const
{
    return m_compressionLevel;
}

void CompressionOptions::setCompressionLevel(int level)
{
    m_compressionLevel = level;
}

ulong CompressionOptions::volumeSize() const
{
    return m_volumeSize;
}

void CompressionOptions::setVolumeSize(ulong size)
{
    m_volumeSize = size;
}

QString CompressionOptions::compressionMethod() const
{
    return m_compressionMethod;
}

void CompressionOptions::setCompressionMethod(const QString &method)
{
    m_compressionMethod = method;
}

QString CompressionOptions::encryptionMethod() const
{
    return m_encryptionMethod;
}

void CompressionOptions::setEncryptionMethod(const QString &method)
{
    m_encryptionMethod = method;
}

QString CompressionOptions::globalWorkDir() const
{
    return m_globalWorkDir;
}

void CompressionOptions::setGlobalWorkDir(const QString &workDir)
{
    m_globalWorkDir = workDir;
}

QDebug operator<<(QDebug d, const CompressionOptions &options)
{
    d.nospace() << "(encryption hint: " << options.encryptedArchiveHint();
    if (!options.compressionMethod().isEmpty()) {
        d.nospace() << ", compression method: " << options.compressionMethod();
    }
    if (!options.encryptionMethod().isEmpty()) {
        d.nospace() << ", encryption method: " << options.encryptionMethod();
    }
    if (!options.globalWorkDir().isEmpty()) {
        d.nospace() << ", global work dir: " << options.globalWorkDir();
    }
    d.nospace() << ", compression level: " << options.compressionLevel();
    d.nospace() << ", volume size: " << options.volumeSize();
    d.nospace() << ")";
    return d.space();
}

QDebug operator<<(QDebug d, ExtractionOptions options)
{
    d.nospace() << "(encryption hint: " << options.encryptedArchiveHint();
    d.nospace() << ", preserve paths: " << options.preservePaths();
    d.nospace() << ", drag and drop: " << options.isDragAndDropEnabled();
    d.nospace() << ", always temp dir: " << options.alwaysUseTempDir();
    d.nospace() << ")";
    return d.space();
}

}
