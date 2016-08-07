/*
 * Copyright (c) 2016 Ragnar Thomsen <rthomsen6@gmail.com>
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

#include "libzipplugin.h"
#include "ark_debug.h"
#include "kerfuffle/kerfuffle_export.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QDataStream>
#include <QDateTime>
#include <QDir>
#include <QFile>

K_PLUGIN_FACTORY_WITH_JSON(LibZipPluginFactory, "kerfuffle_libzip.json", registerPlugin<LibzipPlugin>();)

LibzipPlugin::LibzipPlugin(QObject *parent, const QVariantList & args)
    : ReadWriteArchiveInterface(parent, args)
{
    qCDebug(ARK) << "Initializing libzip plugin";
}

LibzipPlugin::~LibzipPlugin()
{
}

bool LibzipPlugin::list()
{
    qCDebug(ARK) << "Listing archive contents for:" << QFile::encodeName(filename());
    zip_t *archive;
    int err;
    zip_error_t error;

    archive = zip_open(QFile::encodeName(filename()), ZIP_RDONLY, &err);
    zip_error_init_with_code(&error, err);

    if (archive == NULL) {
        qCWarning(ARK) << "Failed to open archive";
        qCWarning(ARK) << "Error code:" << err;
        qCWarning(ARK) << "Error string:" << zip_error_strerror(&error);
        return false;
    }

    m_comment = QString::fromUtf8(zip_get_archive_comment(archive, 0, ZIP_FL_ENC_GUESS));

    zip_int64_t nof_entries = zip_get_num_entries(archive, 0);
    qCDebug(ARK) << "Found entries:" << nof_entries;

    for (zip_int64_t i = 0; i < nof_entries; i++) {

        ArchiveEntry e;
        e[FileName] = QDir::fromNativeSeparators(QString::fromUtf8(zip_get_name(archive, i, ZIP_FL_ENC_GUESS)));
        e[InternalID] = e[FileName];

        if (e[FileName].toString().endsWith(QLatin1Char('/'))) {
            e[IsDirectory] = true;
        } else {
            e[IsDirectory] = false;
        }

        struct zip_stat sb;
        if (zip_stat_index(archive, i, ZIP_FL_ENC_GUESS, &sb)) {
            qCDebug(ARK) << "failed to read stat";
            return false;
        }

        if (sb.valid & ZIP_STAT_MTIME) {
            e[Timestamp] = QDateTime::fromTime_t(sb.mtime);
        }
        if (sb.valid & ZIP_STAT_SIZE) {
            e[Size] = (qlonglong)sb.size;
        }
        if (sb.valid & ZIP_STAT_COMP_SIZE) {
            e[CompressedSize] = (qlonglong)sb.comp_size;
        }
        if (sb.valid & ZIP_STAT_CRC) {
            e[IsDirectory].toBool() ? e[CRC] = QString() : e[CRC] = QString::number((qulonglong)sb.crc, 16).toUpper();
        }
        if (sb.valid & ZIP_STAT_COMP_METHOD) {
            switch(sb.comp_method) {
                case ZIP_CM_STORE:
                    e[Method] = QStringLiteral("Store");
                    break;
                case ZIP_CM_DEFLATE:
                    e[Method] = QStringLiteral("Deflate");
                    break;
                case ZIP_CM_DEFLATE64:
                    e[Method] = QStringLiteral("Deflate64");
                    break;
            }
        }
        if (sb.valid & ZIP_STAT_ENCRYPTION_METHOD) {
            if (sb.encryption_method != ZIP_EM_NONE) {
                e[IsPasswordProtected] = true;
            }
        }

        emit entry(e);
        emit progress(float(i + 1) / nof_entries);
    }

    zip_discard(archive);
    return true;
}

bool LibzipPlugin::addFiles(const QStringList &files, const CompressionOptions &options)
{
    Q_UNUSED(files)
    Q_UNUSED(options)
    return false;
}

bool LibzipPlugin::deleteFiles(const QList<QVariant> &files)
{
    Q_UNUSED(files)
    return false;
}

bool LibzipPlugin::addComment(const QString& comment)
{
    Q_UNUSED(comment)
    return false;
}

bool LibzipPlugin::testArchive()
{
    qCDebug(ARK) << "Testing archive";
    return false;
}

bool LibzipPlugin::doKill()
{
    qCDebug(ARK) << "Killing";
    return true;
}

bool LibzipPlugin::copyFiles(const QVariantList& files, const QString& destinationDirectory, const ExtractionOptions& options)
{
    qCDebug(ARK) << "Extracting files to:" << destinationDirectory;

    const bool extractAll = files.isEmpty();
    const bool removeRootNode = options.value(QStringLiteral("RemoveRootNode"), QVariant()).toBool();

    struct zip *archive;
    int err;
    zip_error_t error;

    archive = zip_open(QFile::encodeName(filename()), ZIP_RDONLY, &err);
    zip_error_init_with_code(&error, err);

    if (archive == NULL) {
        qCWarning(ARK) << "Failed to open archive";
        qCWarning(ARK) << "Error code:" << err;
        qCWarning(ARK) << "Error string:" << zip_error_strerror(&error);
        return false;
    }

    qlonglong totalCount;
    extractAll ? totalCount = zip_get_num_entries(archive, 0) : totalCount = files.size();

    if (extractAll) {
        // We extract all entries.
        for (qlonglong i = 0; i < totalCount; i++) {
            if (!extractEntry(archive,
                              QDir::fromNativeSeparators(QString::fromUtf8(zip_get_name(archive, i, ZIP_FL_ENC_GUESS))),
                              QString(),
                              destinationDirectory,
                              options.value(QStringLiteral("PreservePaths")).toBool())) {
                qCDebug(ARK) << "Extraction failed";
                return false;
            }
            emit progress(float(i + 1) / totalCount);
        }
    } else {
        // We extract only the entries in files.
        qulonglong i = 0;
        foreach (const QVariant &v, files) {
            //qCDebug(ARK) << "Extracting:" << v.value<fileRootNodePair>().file;
            if (!extractEntry(archive,
                              v.value<fileRootNodePair>().file,
                              removeRootNode ? v.value<fileRootNodePair>().rootNode : QString(),
                              destinationDirectory,
                              options.value(QStringLiteral("PreservePaths")).toBool())) {
                qCDebug(ARK) << "Extraction failed";
                return false;
            }
            emit progress(float(++i) / totalCount);
        }
    }

    zip_discard(archive);
    return true;
}

bool LibzipPlugin::extractEntry(zip_t *archive, const QString &entry, const QString &rootNode, const QString &destDir, bool preservePaths)
{
    const bool isDirectory = entry.endsWith(QLatin1Char('/'));

    // Add trailing slash to destDir if not present.
    QString destDirCorrected(destDir);
    if (!destDir.endsWith(QLatin1Char('/'))) {
        destDirCorrected.append(QLatin1Char('/'));
    }

    QString destination;
    if (preservePaths) {
        if (rootNode.isEmpty()) {
            destination = QStringLiteral("%1%2").arg(destDirCorrected).arg(entry);
        } else {
            //qCDebug(ARK) << "Removing rootnode:" << rootNode;
            QString truncatedEntry = entry;
            truncatedEntry.remove(0, rootNode.size());
            destination = QStringLiteral("%1%2").arg(destDirCorrected).arg(truncatedEntry);
        }
    } else {
        if (isDirectory) {
            qCDebug(ARK) << "Skipping directory:" << entry;
            return true;
        }
        destination = QStringLiteral("%1%2").arg(destDirCorrected).arg(QFileInfo(entry).fileName());
    }

    if (isDirectory) {
        //qCDebug(ARK) << "Creating directory:" << QFileInfo(destination).path();
        if (!QDir().mkpath(QFileInfo(destination).path())) {
            qCDebug(ARK) << "Failed to create directory:" << QFileInfo(destination).path();
            return false;
        }
        return true;
    }

    if (!QDir().mkpath(QFileInfo(destination).path())) {
        qCDebug(ARK) << "Failed to create parent directory:" << QFileInfo(destination).path();
    }

    zip_file *zf = zip_fopen(archive, entry.toUtf8(), 0);
    if (!zf) {
        qCDebug(ARK) << "Failed to open file";
        return false;
    }

    //qCDebug(ARK) << "Writing to:" << destination;
    QFile file(destination);
    if (!file.open(QIODevice::WriteOnly)) {
        qCDebug(ARK) << "Failed to open file for writing";
        return false;
    }

    QDataStream out(&file);

    struct zip_stat sb;
    if (zip_stat(archive, entry.toUtf8(), 0, &sb) != 0) {
        qCDebug(ARK) << "Stat failed";
        return false;
    }

    qulonglong sum = 0;
    char buf[1000];
    int len;
    while (sum != sb.size) {
        len = zip_fread(zf, buf, 1000);
        if (len < 0) {
            qCDebug(ARK) << "Failed to read data";
            return false;
        }
        //qCDebug(ARK) << "Read:" << len;
        if (out.writeRawData(buf, len) != len) {
            qCDebug(ARK) << "Failed to write data";
            return false;
        }
        //qCDebug(ARK) << "Wrote:" << len;

        sum += len;
    }
    return true;
}

#include "libzipplugin.moc"
