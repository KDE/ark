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

#ifndef OPTIONS_H
#define OPTIONS_H

#include "kerfuffle_export.h"

#include <QDebug>

namespace Kerfuffle
{

class KERFUFFLE_EXPORT Options
{
public:

    bool encryptedArchiveHint() const;
    void setEncryptedArchiveHint(bool encrypted);

private:

    bool m_encryptedArchiveHint = false;
};

class KERFUFFLE_EXPORT CompressionOptions : public Options
{
public:

    /**
     * @return Whether a custom compression level has been set in the options.
     * If false, the default level from the ArchiveFormat should be used instead.
     * @see compressionLevel()
     */
    bool isCompressionLevelSet() const;

    /**
     * @return Whether a custom volume size has been set in the options.
     * If false, the default size from the ArchiveFormat should be used instead.
     * @see compressionLevel()
     */
    bool isVolumeSizeSet() const;

    int compressionLevel() const;
    void setCompressionLevel(int level);
    ulong volumeSize() const;
    void setVolumeSize(ulong size);
    QString compressionMethod() const;
    void setCompressionMethod(const QString &method);
    QString encryptionMethod() const;
    void setEncryptionMethod(const QString &method);

    /**
     * The working directory of an AddJob.
     * All the path names of new files will be relative to this directory.
     */
    QString globalWorkDir() const;

    /**
     * Sets the global working directory to @p workDir.
     * All interfaces should set this before an AddJob or CreateJob.
     */
    void setGlobalWorkDir(const QString &workDir);

private:
    int m_compressionLevel = -1;
    ulong m_volumeSize = 0;
    QString m_compressionMethod;
    QString m_encryptionMethod;
    QString m_globalWorkDir;
};

class KERFUFFLE_EXPORT ExtractionOptions : public Options
{
public:

    bool preservePaths() const;
    void setPreservePaths(bool preservePaths);
    bool isDragAndDropEnabled() const;
    void setDragAndDropEnabled(bool enabled);
    bool alwaysUseTempDir() const;
    void setAlwaysUseTempDir(bool alwaysUseTempDir);

private:

    bool m_preservePaths = true;
    bool m_dragAndDrop = false;
    bool m_alwaysUseTempDir = false;
};

QDebug KERFUFFLE_EXPORT operator<<(QDebug d, const CompressionOptions &options);
QDebug KERFUFFLE_EXPORT operator<<(QDebug d, const ExtractionOptions &options);

}

Q_DECLARE_METATYPE(Kerfuffle::CompressionOptions)
Q_DECLARE_METATYPE(Kerfuffle::ExtractionOptions)

#endif
