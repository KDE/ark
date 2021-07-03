/*
 * Copyright (c) 2009  Raphael Kubo da Costa <rakuco@FreeBSD.org>
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

#include "singlefileplugin.h"
#include "ark_debug.h"
#include "queries.h"

#include <QFile>
#include <QFileInfo>

#include <karchive_version.h>
#if KARCHIVE_VERSION >= QT_VERSION_CHECK(5, 85, 0)
#include <KCompressionDevice>
#else
#include <KFilterDev>
#endif
#include <KLocalizedString>

LibSingleFileInterface::LibSingleFileInterface(QObject *parent, const QVariantList & args)
        : Kerfuffle::ReadOnlyArchiveInterface(parent, args)
{
    qCDebug(ARK) << "Loaded singlefile plugin";
}

LibSingleFileInterface::~LibSingleFileInterface()
{
}

bool LibSingleFileInterface::extractFiles(const QVector<Kerfuffle::Archive::Entry*> &files, const QString &destinationDirectory, const Kerfuffle::ExtractionOptions &options)
{
    Q_UNUSED(files)
    Q_UNUSED(options)

    QString outputFileName = destinationDirectory;
    if (!destinationDirectory.endsWith(QLatin1Char('/'))) {
        outputFileName += QLatin1Char('/');
    }
    outputFileName += uncompressedFileName();

    outputFileName = overwriteFileName(outputFileName);
    if (outputFileName.isEmpty()) {
        return true;
    }

    qCDebug(ARK) << "Extracting to" << outputFileName;

    QFile outputFile(outputFileName);
    if (!outputFile.open(QIODevice::WriteOnly)) {
        qCCritical(ARK) << "Failed to open output file" << outputFile.errorString();
        Q_EMIT error(xi18nc("@info", "Ark could not extract <filename>%1</filename>.", outputFile.fileName()));

        return false;
    }

#if KARCHIVE_VERSION >= QT_VERSION_CHECK(5, 85, 0)
    KCompressionDevice *device = new KCompressionDevice(filename(), KCompressionDevice::compressionTypeForMimeType(m_mimeType));
#else
    KCompressionDevice *device = new KCompressionDevice(filename(), KFilterDev::compressionTypeForMimeType(m_mimeType));
#endif
    if (!device) {
        qCCritical(ARK) << "Could not create KCompressionDevice";
        Q_EMIT error(xi18nc("@info", "Ark could not open <filename>%1</filename> for extraction.", filename()));

        return false;
    }

    device->open(QIODevice::ReadOnly);

    qint64 bytesRead;
    QByteArray dataChunk(1024*16, '\0');   // 16Kb

    while (true) {
        bytesRead = device->read(dataChunk.data(), dataChunk.size());

        if (bytesRead == -1) {
            Q_EMIT error(xi18nc("@info", "There was an error while reading <filename>%1</filename> during extraction.", filename()));
            break;
        } else if (bytesRead == 0) {
            break;
        }

        outputFile.write(dataChunk.data(), bytesRead);
    }

    delete device;

    return true;
}

bool LibSingleFileInterface::list()
{
    qCDebug(ARK) << "Listing archive contents";

    Kerfuffle::Archive::Entry *e = new Kerfuffle::Archive::Entry();
    connect(this, &QObject::destroyed, e, &QObject::deleteLater);
    e->setProperty("fullPath", uncompressedFileName());
    e->setProperty("compressedSize", QFileInfo(filename()).size());
    Q_EMIT entry(e);

    return true;
}

QString LibSingleFileInterface::overwriteFileName(QString& filename)
{
    QString newFileName(filename);

    while (QFile::exists(newFileName)) {
        Kerfuffle::OverwriteQuery query(newFileName);

        query.setMultiMode(false);
        Q_EMIT userQuery(&query);
        query.waitForResponse();

        if ((query.responseCancelled()) || (query.responseSkip())) {
            return QString();
        } else if (query.responseOverwrite()) {
            break;
        } else if (query.responseRename()) {
            newFileName = query.newFilename();
        }
    }

    return newFileName;
}

const QString LibSingleFileInterface::uncompressedFileName() const
{
    QString uncompressedName(QFileInfo(filename()).fileName());

    // Bug 252701: For .svgz just remove the terminal "z".
    if (uncompressedName.endsWith(QLatin1String(".svgz"), Qt::CaseInsensitive)) {
        uncompressedName.chop(1);
        return uncompressedName;
    }

    for (const QString & extension : qAsConst(m_possibleExtensions)) {
        qCDebug(ARK) << extension;

        if (uncompressedName.endsWith(extension, Qt::CaseInsensitive)) {
            uncompressedName.chop(extension.size());
            return uncompressedName;
        }
    }

    return uncompressedName + QStringLiteral( ".uncompressed" );
}

bool LibSingleFileInterface::testArchive()
{
    return false;
}

