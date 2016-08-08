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
#include "kerfuffle/queries.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QDataStream>
#include <QDateTime>
#include <QDir>
#include <QFile>

K_PLUGIN_FACTORY_WITH_JSON(LibZipPluginFactory, "kerfuffle_libzip.json", registerPlugin<LibzipPlugin>();)

LibzipPlugin::LibzipPlugin(QObject *parent, const QVariantList & args)
    : ReadWriteArchiveInterface(parent, args)
    , m_abortOperation(false)
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
    int errcode;
    zip_error_t err;

    // Open archive.
    archive = zip_open(QFile::encodeName(filename()), ZIP_RDONLY, &errcode);
    zip_error_init_with_code(&err, errcode);
    if (archive == NULL) {
        qCCritical(ARK) << "Failed to open archive. Code:" << errcode;
        emit error(xi18n("Failed to open archive: %1", QString::fromUtf8(zip_error_strerror(&err))));
        return false;
    }

    // Fetch archive comment.
    m_comment = QString::fromUtf8(zip_get_archive_comment(archive, 0, ZIP_FL_ENC_GUESS));

    // Get number of archive entries.
    qlonglong nofEntries = zip_get_num_entries(archive, 0);
    qCDebug(ARK) << "Found entries:" << nofEntries;

    // Loop through all archive entries.
    for (zip_int64_t i = 0; i < nofEntries; i++) {

        if (m_abortOperation) {
            break;
        }

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
        emit progress(float(i + 1) / nofEntries);
    }
    m_abortOperation = false;

    zip_close(archive);
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
    qCDebug(ARK) << "Killing action...";
    m_abortOperation = true;
    return true;
}

bool LibzipPlugin::copyFiles(const QVariantList& files, const QString& destinationDirectory, const ExtractionOptions& options)
{
    qCDebug(ARK) << "Extracting files to:" << destinationDirectory;

    const bool extractAll = files.isEmpty();
    const bool removeRootNode = options.value(QStringLiteral("RemoveRootNode"), QVariant()).toBool();

    struct zip *archive;
    int errcode;
    zip_error_t err;

    // Open archive.
    archive = zip_open(QFile::encodeName(filename()), ZIP_RDONLY, &errcode);
    zip_error_init_with_code(&err, errcode);
    if (archive == NULL) {
        qCCritical(ARK) << "Failed to open archive. Code:" << errcode;
        emit error(xi18n("Failed to open archive: %1", QString::fromUtf8(zip_error_strerror(&err))));
        return false;
    }

    // Set password if known.
    if (!password().isEmpty()) {
        qCDebug(ARK) << "Password already known. Setting...";
        zip_set_default_password(archive, password().toUtf8());
    }

    // Get number of archive entries.
    qlonglong nofEntries;
    extractAll ? nofEntries = zip_get_num_entries(archive, 0) : nofEntries = files.size();

    // Extract entries.
    m_overwriteAll = false; // Whether to overwrite all files
    m_skipAll = false; // Whether to skip all files
    if (extractAll) {
        // We extract all entries.
        for (qlonglong i = 0; i < nofEntries; i++) {
            if (m_abortOperation) {
                break;
            }
            if (!extractEntry(archive,
                              QDir::fromNativeSeparators(QString::fromUtf8(zip_get_name(archive, i, ZIP_FL_ENC_GUESS))),
                              QString(),
                              destinationDirectory,
                              options.value(QStringLiteral("PreservePaths")).toBool())) {
                qCDebug(ARK) << "Extraction failed";
                return false;
            }
            emit progress(float(i + 1) / nofEntries);
        }
    } else {
        // We extract only the entries in files.
        qulonglong i = 0;
        foreach (const QVariant &v, files) {
            if (m_abortOperation) {
                break;
            }
            //qCDebug(ARK) << "Extracting:" << v.value<fileRootNodePair>().file;
            if (!extractEntry(archive,
                              v.value<fileRootNodePair>().file,
                              removeRootNode ? v.value<fileRootNodePair>().rootNode : QString(),
                              destinationDirectory,
                              options.value(QStringLiteral("PreservePaths")).toBool())) {
                qCDebug(ARK) << "Extraction failed";
                return false;
            }
            emit progress(float(++i) / nofEntries);
        }
    }
    m_abortOperation = false;

    zip_close(archive);
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

    // Remove rootnode if supplied and set destination path.
    QString destination;
    if (preservePaths) {
        if (rootNode.isEmpty()) {
            destination = destDirCorrected + entry;
        } else {
            //qCDebug(ARK) << "Removing rootnode:" << rootNode;
            QString truncatedEntry = entry;
            truncatedEntry.remove(0, rootNode.size());
            destination = destDirCorrected + truncatedEntry;
        }
    } else {
        if (isDirectory) {
            qCDebug(ARK) << "Skipping directory:" << entry;
            return true;
        }
        destination = destDirCorrected + QFileInfo(entry).fileName();
    }

    // Create parent directories for files. For directories create them.
    if (!QDir().mkpath(QFileInfo(destination).path())) {
        qCDebug(ARK) << "Failed to create directory:" << QFileInfo(destination).path();
        emit error(xi18n("Failed to create directory: %1", QFileInfo(destination).path()));
        return false;
    }

    if (isDirectory) {
        return true;
    }

    // Handle existing destination files.
    QString renamedEntry = entry;
    while (!m_overwriteAll && QFileInfo(destination).exists()) {
        if (m_skipAll) {
            return true;
        } else {
            Kerfuffle::OverwriteQuery query(renamedEntry);
            emit userQuery(&query);
            query.waitForResponse();

            if (query.responseCancelled()) {
                return false;
            } else if (query.responseSkip()) {
                return true;
            } else if (query.responseAutoSkip()) {
                m_skipAll = true;
                return true;
            } else if (query.responseRename()) {
                const QString newName(query.newFilename());
                destination = QFileInfo(destination).path() + QLatin1Char('/') + QFileInfo(newName).fileName();
                renamedEntry = QFileInfo(entry).path() + QLatin1Char('/') + QFileInfo(newName).fileName();
            } else if (query.responseOverwriteAll()) {
                m_overwriteAll = true;
                break;
            } else if (query.responseOverwrite()) {
                break;
            }
        }
    }

    // Handle password-protected files.
    zip_file *zf = NULL;
    bool firstTry = true;
    while (!zf) {
        zf = zip_fopen(archive, entry.toUtf8(), 0);
        if (zf) {
            break;
        } else if (zip_error_code_zip(zip_get_error(archive)) == ZIP_ER_NOPASSWD ||
                   zip_error_code_zip(zip_get_error(archive)) == ZIP_ER_WRONGPASSWD) {
            Kerfuffle::PasswordNeededQuery query(filename(), !firstTry);
            emit userQuery(&query);
            query.waitForResponse();

            if (query.responseCancelled()) {
                emit cancelled();
                return false;
            }
            setPassword(query.password());

            if (zip_set_default_password(archive, password().toUtf8())) {
                qCDebug(ARK) << "Failed to set password for:" << entry;
            }
            firstTry = false;
        } else {
            qCDebug(ARK) << "Failed to open file:" << zip_strerror(archive) << zip_error_code_zip(zip_get_error(archive));
            return false;
        }
    }

    //qCDebug(ARK) << "Writing to:" << destination;
    QFile file(destination);
    if (!file.open(QIODevice::WriteOnly)) {
        qCDebug(ARK) << "Failed to open file for writing";
        emit error(xi18n("Failed to open file for writing: %1", destination));
        return false;
    }

    QDataStream out(&file);

    // Get statistic for entry. Used below to get entry size.
    struct zip_stat sb;
    if (zip_stat(archive, entry.toUtf8(), 0, &sb) != 0) {
        qCDebug(ARK) << "Stat failed";
        return false;
    }

    // Write archive entry to file. We use a read/write buffer of 1000 chars.
    qulonglong sum = 0;
    char buf[1000];
    int len;
    while (sum != sb.size) {
        len = zip_fread(zf, buf, 1000);
        if (len < 0) {
            qCDebug(ARK) << "Failed to read data";
            emit error(xi18n("Failed to read data for entry: %1", entry));
            return false;
        }
        //qCDebug(ARK) << "Read:" << len;
        if (out.writeRawData(buf, len) != len) {
            qCDebug(ARK) << "Failed to write data";
            emit error(xi18n("Failed to write data for entry: %1", entry));
            return false;
        }
        //qCDebug(ARK) << "Wrote:" << len;

        sum += len;
    }
    return true;
}

#include "libzipplugin.moc"
