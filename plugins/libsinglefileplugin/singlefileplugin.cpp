/*
 * Copyright (c) 2009  Raphael Kubo da Costa <kubito@gmail.com>
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
#include "kerfuffle/archivefactory.h"

#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QString>

#include <KDebug>
#include <KFilterDev>
#include <KLocale>

LibSingleFileInterface::LibSingleFileInterface(const QString & filename, QObject *parent)
        : Kerfuffle::ReadOnlyArchiveInterface(filename, parent)
{
    kDebug(1601) << filename;
}

LibSingleFileInterface::~LibSingleFileInterface()
{
    kDebug(1601);
}

bool LibSingleFileInterface::copyFiles(const QList<QVariant> & files, const QString & destinationDirectory, Kerfuffle::ExtractionOptions options)
{
    Q_UNUSED(files);
    Q_UNUSED(options);

    QString outputFileName = destinationDirectory;
    if (!destinationDirectory.endsWith('/'))
        outputFileName += '/';
    outputFileName += uncompressedFileName();

    outputFileName = overwriteFileName(outputFileName);
    if (outputFileName.isEmpty())
        return true;

    kDebug(1601) << "Extracting to" << outputFileName;

    QFile outputFile(outputFileName);
    if (!outputFile.open(QIODevice::WriteOnly)) {
        kDebug(1601) << "Failed to open output file" << outputFile.errorString();
        error(i18n("Ark could not extract %1.", outputFile.fileName()));

        return false;
    }

    QIODevice *device = KFilterDev::deviceForFile(filename(), m_mimeType, false);
    if (!device) {
        kDebug(1601) << "Could not create KFilterDev";
        error(i18n("Ark could not open %1 for extraction.", filename()));

        return false;
    }

    device->open(QIODevice::ReadOnly);

    qint64 bytesRead;
    QByteArray dataChunk(1024*16, '\0');   // 16Kb

    while (true) {
        bytesRead = device->read(dataChunk.data(), dataChunk.size());

        if (bytesRead == -1) {
            error(i18n("There was an error while reading %1 during extraction.", filename()));
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
    kDebug(1601);

    const QString filename = uncompressedFileName();

    Kerfuffle::ArchiveEntry e;

    e[Kerfuffle::FileName] = filename;
    e[Kerfuffle::InternalID] = filename;

    entry(e);

    return true;
}

QString LibSingleFileInterface::overwriteFileName(QString& filename)
{
    QString newFileName(filename);

    while (QFile::exists(newFileName)) {
        Kerfuffle::OverwriteQuery query(newFileName);

        query.setMultiMode(false);
        userQuery(&query);
        query.waitForResponse();

        if ((query.responseCancelled()) || (query.responseSkip()))
            return QString();
        else if (query.responseOverwrite())
            break;
        else if (query.responseRename())
            newFileName = query.newFilename();
    }

    return newFileName;
}

const QString LibSingleFileInterface::uncompressedFileName() const
{
    QString uncompressedName(QFileInfo(filename()).fileName());

    foreach(const QString & extension, m_possibleExtensions) {
        kDebug(1601) << extension;

        if (uncompressedName.endsWith(extension, Qt::CaseInsensitive)) {
            uncompressedName.chop(extension.size());
            return uncompressedName;
        }
    }

    return uncompressedName + ".uncompressed";
}

#include "singlefileplugin.moc"
