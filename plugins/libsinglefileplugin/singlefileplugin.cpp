/*
    SPDX-FileCopyrightText: 2009 Raphael Kubo da Costa <rakuco@FreeBSD.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "singlefileplugin.h"
#include "ark_debug.h"
#include "queries.h"

#include <QFile>
#include <QFileInfo>

#include <karchive_version.h>
#include <KCompressionDevice>
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

    KCompressionDevice *device = new KCompressionDevice(filename(), KCompressionDevice::compressionTypeForMimeType(m_mimeType));
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

    for (const QString & extension : std::as_const(m_possibleExtensions)) {
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

