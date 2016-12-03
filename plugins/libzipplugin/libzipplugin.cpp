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
#include "queries.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QDataStream>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
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
    for (qlonglong i = 0; i < nofEntries; i++) {

        if (m_abortOperation) {
            break;
        }

        emitEntryForIndex(archive, i);
        emit progress(float(i + 1) / nofEntries);
    }
    m_abortOperation = false;

    zip_close(archive);
    return true;
}

bool LibzipPlugin::addFiles(const QVector<Archive::Entry*> &files, const Archive::Entry *destination, const CompressionOptions& options, uint numberOfEntriesToAdd)
{
    qulonglong totalCount = numberOfEntriesToAdd;

    zip_t *archive;
    int errcode;
    zip_error_t err;

    // Open archive.
    archive = zip_open(QFile::encodeName(filename()), ZIP_CREATE, &errcode);
    zip_error_init_with_code(&err, errcode);
    if (archive == NULL) {
        qCCritical(ARK) << "Failed to open archive. Code:" << errcode;
        emit error(xi18n("Failed to open archive: %1", QString::fromUtf8(zip_error_strerror(&err))));
        return false;
    }

    qulonglong i = 0;
    foreach (const Archive::Entry* e, files) {

        if (m_abortOperation) {
            break;
        }

        // If entry is a directory, traverse and add all its files and subfolders.
        if (QFileInfo(e->fullPath()).isDir()) {

            if (!writeEntry(archive, e->fullPath(), true)) {
                return false;
            }

            QDirIterator it(e->fullPath(),
                            QDir::AllEntries | QDir::Readable |
                            QDir::Hidden | QDir::NoDotAndDotDot,
                            QDirIterator::Subdirectories);

            while (!m_abortOperation && it.hasNext()) {
                QString path = it.next();

                if (QFileInfo(path).isDir()) {

                    if (!writeEntry(archive, path, true)) {
                        return false;
                    }

                } else {

                    if (!writeEntry(archive, path)) {
                        return false;
                    }

                }
                emit progress(float(++i) / totalCount);
            }

        } else {

            if (!writeEntry(archive, e->fullPath())) {
                return false;
            }

        }

        emit progress(float(++i) / totalCount);
    }
    qCDebug(ARK) << "Added" << i << "entries";
    m_abortOperation = false;

    zip_close(archive);
    return true;
}

bool LibzipPlugin::writeEntry(zip_t *archive, const QString &file, bool isDir)
{
    qlonglong index;
    if (isDir) {
        index = zip_dir_add(archive, file.toUtf8(), ZIP_FL_ENC_GUESS);
        if (index == -1) {
            // If directory already exists in archive, we get an error.
            qCWarning(ARK) << "Failed to add dir " << file << ":" << zip_strerror(archive);
            return true;
        }
    } else {
        zip_source_t *src = zip_source_file(archive, QFile::encodeName(file).constData(), 0, -1);

        index = zip_file_add(archive, file.toUtf8(), src, ZIP_FL_ENC_GUESS | ZIP_FL_OVERWRITE);
        if (index == -1) {
            qCCritical(ARK) << "Could not add entry" << file << ":" << zip_strerror(archive);
            emit error(xi18n("Failed to add entry: %1", QString::fromUtf8(zip_strerror(archive))));
            return false;
        }
    }
    emitEntryForIndex(archive, index);

    return true;
}

bool LibzipPlugin::emitEntryForIndex(zip_t *archive, qlonglong index)
{
    zip_stat_t sb;
    if (zip_stat_index(archive, index, ZIP_FL_ENC_GUESS, &sb)) {
        qCCritical(ARK) << "Failed to read stat for index" << index;
        return false;
    }

    auto e = new Archive::Entry();

    if (sb.valid & ZIP_STAT_NAME) {
        e->setFullPath(QString::fromUtf8(sb.name));
    }

    //e->isDir().toString().endsWith(QDir::separator()) ? e[IsDirectory] = true : e[IsDirectory] = false;

    if (e->fullPath(PathFormat::WithTrailingSlash).endsWith(QDir::separator())) {
        e->setProperty("isDirectory", true);
    }

    if (sb.valid & ZIP_STAT_MTIME) {
        e->setProperty("timestamp", QDateTime::fromTime_t(sb.mtime));
    }
    if (sb.valid & ZIP_STAT_SIZE) {
        e->setProperty("size", (qulonglong)sb.size);
    }
    if (sb.valid & ZIP_STAT_COMP_SIZE) {
        e->setProperty("compressedSize", (qlonglong)sb.comp_size);
    }
    if (sb.valid & ZIP_STAT_CRC) {
        if (!e->isDir()) {
            e->setProperty("CRC", QString::number((qulonglong)sb.crc, 16).toUpper());
        }
    }
    if (sb.valid & ZIP_STAT_COMP_METHOD) {
        qCDebug(ARK) << "compmethod:" << sb.comp_method;
        switch(sb.comp_method) {
            case ZIP_CM_STORE:
                e->setProperty("method", QStringLiteral("Store"));
                emit compressionMethodFound(QStringLiteral("Store"));
                break;
            case ZIP_CM_DEFLATE:
                e->setProperty("method", QStringLiteral("Deflate"));
                emit compressionMethodFound(QStringLiteral("Deflate"));
                break;
            case ZIP_CM_DEFLATE64:
                e->setProperty("method", QStringLiteral("Deflate64"));
                emit compressionMethodFound(QStringLiteral("Deflate64"));
                break;
            case ZIP_CM_BZIP2:
                e->setProperty("method", QStringLiteral("BZip2"));
                emit compressionMethodFound(QStringLiteral("BZip2"));
                break;
            case ZIP_CM_LZMA:
                e->setProperty("method", QStringLiteral("LZMA"));
                emit compressionMethodFound(QStringLiteral("LZMA"));
                break;
        }
    }
    if (sb.valid & ZIP_STAT_ENCRYPTION_METHOD) {
        if (sb.encryption_method != ZIP_EM_NONE) {
            e->setProperty("isPasswordProtected", true);
            switch(sb.encryption_method) {
                case ZIP_EM_TRAD_PKWARE:
                    emit encryptionMethodFound(QStringLiteral("ZipCrypto"));
                    break;
                case ZIP_EM_AES_128:
                    emit encryptionMethodFound(QStringLiteral("AES128"));
                    break;
                case ZIP_EM_AES_192:
                    emit encryptionMethodFound(QStringLiteral("AES192"));
                    break;
                case ZIP_EM_AES_256:
                    emit encryptionMethodFound(QStringLiteral("AES256"));
                    break;
            }
        }
    }

    emit entry(e);

    return true;
}

bool LibzipPlugin::deleteFiles(const QVector<Archive::Entry*> &files)
{
    zip_t *archive;
    int errcode;
    zip_error_t err;

    // Open archive.
    archive = zip_open(QFile::encodeName(filename()), 0, &errcode);
    zip_error_init_with_code(&err, errcode);
    if (archive == NULL) {
        qCCritical(ARK) << "Failed to open archive. Code:" << errcode;
        emit error(xi18n("Failed to open archive: %1", QString::fromUtf8(zip_error_strerror(&err))));
        return false;
    }

    qulonglong i = 0;
    foreach (const Archive::Entry* e, files) {

        if (m_abortOperation) {
            break;
        }

        qlonglong index = zip_name_locate(archive, e->fullPath().toUtf8(), ZIP_FL_ENC_GUESS);
        if (index == -1) {
            qCCritical(ARK) << "Could not find entry to delete:" << e->fullPath();
            emit error(xi18n("Failed to delete entry: %1", e->fullPath()));
            return false;
        }
        if (zip_delete(archive, index) == -1) {
            qCCritical(ARK) << "Could not delete entry" << e->fullPath() << ":" << zip_strerror(archive);
            emit error(xi18n("Failed to delete entry: %1", QString::fromUtf8(zip_strerror(archive))));
            return false;
        }
        emit entryRemoved(e->fullPath());
        emit progress(float(++i) / files.size());
    }
    qCDebug(ARK) << "Deleted" << i << "entries";
    m_abortOperation = false;

    zip_close(archive);
    return true;
}

bool LibzipPlugin::addComment(const QString& comment)
{
    zip_t *archive;
    int errcode;
    zip_error_t err;

    // Open archive.
    archive = zip_open(QFile::encodeName(filename()), 0, &errcode);
    zip_error_init_with_code(&err, errcode);
    if (archive == NULL) {
        qCCritical(ARK) << "Failed to open archive. Code:" << errcode;
        emit error(xi18n("Failed to open archive: %1", QString::fromUtf8(zip_error_strerror(&err))));
        return false;
    }

    // Set archive comment.
    if (zip_set_archive_comment(archive, comment.toUtf8(), comment.length())) {
        qCCritical(ARK) << "Failed to set comment:" << zip_strerror(archive);
        return false;
    }

    zip_close(archive);
    return true;
}

bool LibzipPlugin::testArchive()
{
    qCDebug(ARK) << "Testing archive";

    zip_t *archive;
    int errcode;
    zip_error_t err;

    // Open archive performing extra consistency checks.
    archive = zip_open(QFile::encodeName(filename()), ZIP_CHECKCONS, &errcode);
    zip_error_init_with_code(&err, errcode);
    if (archive == NULL) {
        qCCritical(ARK) << "Failed to open archive:" << zip_error_strerror(&err);
        return false;
    }

    emit testSuccess();
    return true;
}

bool LibzipPlugin::doKill()
{
    qCDebug(ARK) << "Killing operation...";
    m_abortOperation = true;
    return true;
}

bool LibzipPlugin::extractFiles(const QVector<Archive::Entry*> &files, const QString& destinationDirectory, const ExtractionOptions& options)
{
    qCDebug(ARK) << "Extracting files to:" << destinationDirectory;

    const bool extractAll = files.isEmpty();

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
                              options.preservePaths())) {
                qCDebug(ARK) << "Extraction failed";
                return false;
            }
            emit progress(float(i + 1) / nofEntries);
        }
    } else {
        // We extract only the entries in files.
        qulonglong i = 0;
        foreach (const Archive::Entry* e, files) {
            if (m_abortOperation) {
                break;
            }
            if (!extractEntry(archive,
                              e->fullPath(),
                              e->rootNode,
                              destinationDirectory,
                              options.preservePaths())) {
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
    const bool isDirectory = entry.endsWith(QDir::separator());

    // Add trailing slash to destDir if not present.
    QString destDirCorrected(destDir);
    if (!destDir.endsWith(QDir::separator())) {
        destDirCorrected.append(QDir::separator());
    }

    // Remove rootnode if supplied and set destination path.
    QString destination;
    if (preservePaths) {
        if (rootNode.isEmpty()) {
            destination = destDirCorrected + entry;
        } else {
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
                destination = QFileInfo(destination).path() + QDir::separator() + QFileInfo(newName).fileName();
                renamedEntry = QFileInfo(entry).path() + QDir::separator() + QFileInfo(newName).fileName();
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
            qCCritical(ARK) << "Failed to open file:" << zip_strerror(archive) << zip_error_code_zip(zip_get_error(archive));
            return false;
        }
    }

    QFile file(destination);
    if (!file.open(QIODevice::WriteOnly)) {
        qCCritical(ARK) << "Failed to open file for writing";
        emit error(xi18n("Failed to open file for writing: %1", destination));
        return false;
    }

    QDataStream out(&file);

    // Get statistic for entry. Used below to get entry size.
    zip_stat_t sb;
    if (zip_stat(archive, entry.toUtf8(), 0, &sb) != 0) {
        qCCritical(ARK) << "Failed to read stat for entry" << entry;
        return false;
    }

    // Write archive entry to file. We use a read/write buffer of 1000 chars.
    qulonglong sum = 0;
    char buf[1000];
    int len;
    while (sum != sb.size) {
        len = zip_fread(zf, buf, 1000);
        if (len < 0) {
            qCCritical(ARK) << "Failed to read data";
            emit error(xi18n("Failed to read data for entry: %1", entry));
            return false;
        }
        if (out.writeRawData(buf, len) != len) {
            qCCritical(ARK) << "Failed to write data";
            emit error(xi18n("Failed to write data for entry: %1", entry));
            return false;
        }

        sum += len;
    }
    return true;
}

bool LibzipPlugin::moveFiles(const QVector<Archive::Entry*> &files, Archive::Entry *destination, const CompressionOptions &options)
{
    Q_UNUSED(files)
    Q_UNUSED(destination)
    Q_UNUSED(options)
    return false;
}

bool LibzipPlugin::copyFiles(const QVector<Archive::Entry*> &files, Archive::Entry *destination, const CompressionOptions &options)
{
    Q_UNUSED(files)
    Q_UNUSED(destination)
    Q_UNUSED(options)
    return false;
}

#include "libzipplugin.moc"
