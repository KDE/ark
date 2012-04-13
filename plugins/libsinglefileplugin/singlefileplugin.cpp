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
#include "kerfuffle/kerfuffle_export.h"
#include "kerfuffle/queries.h"

#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QString>

#include <KDebug>
#include <KFilterDev>
#include <KLocale>

LibSingleFileInterface::LibSingleFileInterface(QObject *parent, const QVariantList & args)
        : Kerfuffle::ReadOnlyArchiveInterface(parent, args)
{
}

LibSingleFileInterface::~LibSingleFileInterface()
{
}

bool LibSingleFileInterface::copyFiles(const QList<QVariant> & files, const QString & destinationDirectory, Kerfuffle::ExtractionOptions options)
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

    kDebug() << "Extracting to" << outputFileName;

    QFile outputFile(outputFileName);
    if (!outputFile.open(QIODevice::WriteOnly)) {
        kDebug() << "Failed to open output file" << outputFile.errorString();
        emit error(i18nc("@info", "Ark could not extract <filename>%1</filename>.", outputFile.fileName()));

        return false;
    }

    QIODevice *device = KFilterDev::deviceForFile(filename(), m_mimeType, false);
    if (!device) {
        kDebug() << "Could not create KFilterDev";
        emit error(i18nc("@info", "Ark could not open <filename>%1</filename> for extraction.", filename()));

        return false;
    }

    device->open(QIODevice::ReadOnly);

    qint64 bytesRead;
    QByteArray dataChunk(1024*16, '\0');   // 16Kb

    while (true) {
        bytesRead = device->read(dataChunk.data(), dataChunk.size());

        if (bytesRead == -1) {
            emit error(i18nc("@info", "There was an error while reading <filename>%1</filename> during extraction.", filename()));
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
    kDebug();

    const QString filename = uncompressedFileName();

    Kerfuffle::ArchiveEntry e;

    e[Kerfuffle::FileName] = filename;
    e[Kerfuffle::InternalID] = filename;

    emit entry(e);

    return true;
}

QString LibSingleFileInterface::overwriteFileName(QString& filename)
{
    QString newFileName(filename);

    while (QFile::exists(newFileName)) {
        Kerfuffle::OverwriteQuery query(newFileName);

        query.setMultiMode(false);
        emit userQuery(&query);
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

    foreach(const QString & extension, m_possibleExtensions) {
        kDebug() << extension;

        if (uncompressedName.endsWith(extension, Qt::CaseInsensitive)) {
            uncompressedName.chop(extension.size());
            return uncompressedName;
        }
    }

    return uncompressedName + QLatin1String( ".uncompressed" );
}

#include "singlefileplugin.moc"
