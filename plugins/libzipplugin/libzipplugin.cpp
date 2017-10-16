/*
 * Copyright (c) 2017 Ragnar Thomsen <rthomsen6@gmail.com>
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

#include <KIO/Global>
#include <KLocalizedString>
#include <KPluginFactory>

#include <QDataStream>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QThread>

#include <utime.h>

K_PLUGIN_FACTORY_WITH_JSON(LibZipPluginFactory, "kerfuffle_libzip.json", registerPlugin<LibzipPlugin>();)

// This is needed for hooking a C callback to a C++ non-static member
// function.
template <typename T>
struct Callback;
template <typename Ret, typename... Params>
struct Callback<Ret(Params...)> {
    template <typename... Args>
    static Ret callback(Args... args) { return func(args...); }
    static std::function<Ret(Params...)> func;
};
// Initialize the static member.
template <typename Ret, typename... Params>
std::function<Ret(Params...)> Callback<Ret(Params...)>::func;

LibzipPlugin::LibzipPlugin(QObject *parent, const QVariantList & args)
    : ReadWriteArchiveInterface(parent, args)
    , m_overwriteAll(false)
    , m_skipAll(false)
    , m_listAfterAdd(false)
{
    qCDebug(ARK) << "Initializing libzip plugin";
}

LibzipPlugin::~LibzipPlugin()
{
    foreach (const auto e, m_emittedEntries) {
        // Entries might be passed to pending slots, so we just schedule their deletion.
        e->deleteLater();
    }
}

bool LibzipPlugin::list()
{
    qCDebug(ARK) << "Listing archive contents for:" << QFile::encodeName(filename());

    m_numberOfEntries = 0;

    int errcode;
    zip_error_t err;

    // Open archive.
    zip_t *archive = zip_open(QFile::encodeName(filename()), ZIP_RDONLY, &errcode);
    zip_error_init_with_code(&err, errcode);
    if (!archive) {
        qCCritical(ARK) << "Failed to open archive. Code:" << errcode;
        emit error(xi18n("Failed to open archive: %1", QString::fromUtf8(zip_error_strerror(&err))));
        return false;
    }

    // Fetch archive comment.
    m_comment = QString::fromUtf8(zip_get_archive_comment(archive, nullptr, ZIP_FL_ENC_GUESS));

    // Get number of archive entries.
    int nofEntries = zip_get_num_entries(archive, 0);
    qCDebug(ARK) << "Found entries:" << nofEntries;

    // Loop through all archive entries.
    for (int i = 0; i < nofEntries; i++) {

        if (QThread::currentThread()->isInterruptionRequested()) {
            break;
        }

        emitEntryForIndex(archive, i);
        if (m_listAfterAdd) {
            // Start at 50%.
            emit progress(0.5 + (0.5 * float(i + 1) / nofEntries));
        } else {
            emit progress(float(i + 1) / nofEntries);
        }
    }

    zip_close(archive);
    m_listAfterAdd = false;
    return true;
}

bool LibzipPlugin::addFiles(const QVector<Archive::Entry*> &files, const Archive::Entry *destination, const CompressionOptions& options, uint numberOfEntriesToAdd)
{
    Q_UNUSED(numberOfEntriesToAdd)

    int errcode;
    zip_error_t err;

    // Open archive.
    zip_t *archive = zip_open(QFile::encodeName(filename()), ZIP_CREATE, &errcode);
    zip_error_init_with_code(&err, errcode);
    if (!archive) {
        qCCritical(ARK) << "Failed to open archive. Code:" << errcode;
        emit error(xi18n("Failed to open archive: %1", QString::fromUtf8(zip_error_strerror(&err))));
        return false;
    }

    uint i = 0;
    foreach (const Archive::Entry* e, files) {

        if (QThread::currentThread()->isInterruptionRequested()) {
            break;
        }

        // If entry is a directory, traverse and add all its files and subfolders.
        if (QFileInfo(e->fullPath()).isDir()) {

            if (!writeEntry(archive, e->fullPath(), destination, options, true)) {
                return false;
            }

            QDirIterator it(e->fullPath(),
                            QDir::AllEntries | QDir::Readable |
                            QDir::Hidden | QDir::NoDotAndDotDot,
                            QDirIterator::Subdirectories);

            while (!QThread::currentThread()->isInterruptionRequested() && it.hasNext()) {
                QString path = it.next();

                if (QFileInfo(path).isDir()) {
                    if (!writeEntry(archive, path, destination, options, true)) {
                        return false;
                    }
                } else {
                    if (!writeEntry(archive, path, destination, options)) {
                        return false;
                    }
                }
                i++;
            }
        } else {
            if (!writeEntry(archive, e->fullPath(), destination, options)) {
                return false;
            }
        }
        i++;
    }
    qCDebug(ARK) << "Added" << i << "entries";

    // Register the callback function to get progress feedback.
    Callback<void(double)>::func = std::bind(&LibzipPlugin::progressEmitted, this, std::placeholders::_1);
    void (*c_func)(double) = static_cast<decltype(c_func)>(Callback<void(double)>::callback);
    zip_register_progress_callback(archive, c_func);

    qCDebug(ARK) << "Writing entries to disk...";
    if (zip_close(archive)) {
        qCCritical(ARK) << "Failed to write archive";
        emit error(xi18n("Failed to write archive."));
        return false;
    }

    // We list the entire archive after adding files to ensure entry
    // properties are up-to-date.
    m_listAfterAdd = true;
    list();

    return true;
}

void LibzipPlugin::progressEmitted(double pct)
{
    // Go from 0 to 50%. The second half is the subsequent listing.
    emit progress(0.5 * pct);
}

bool LibzipPlugin::writeEntry(zip_t *archive, const QString &file, const Archive::Entry* destination, const CompressionOptions& options, bool isDir)
{
    Q_ASSERT(archive);

    QByteArray destFile;
    if (destination) {
        destFile = QString(destination->fullPath() + file).toUtf8();
    } else {
        destFile = file.toUtf8();
    }

    qlonglong index;
    if (isDir) {
        index = zip_dir_add(archive, destFile, ZIP_FL_ENC_GUESS);
        if (index == -1) {
            // If directory already exists in archive, we get an error.
            qCWarning(ARK) << "Failed to add dir " << file << ":" << zip_strerror(archive);
            return true;
        }
    } else {
        zip_source_t *src = zip_source_file(archive, QFile::encodeName(file).constData(), 0, -1);
        Q_ASSERT(src);

        index = zip_file_add(archive, destFile, src, ZIP_FL_ENC_GUESS | ZIP_FL_OVERWRITE);
        if (index == -1) {
            zip_source_free(src);
            qCCritical(ARK) << "Could not add entry" << file << ":" << zip_strerror(archive);
            emit error(xi18n("Failed to add entry: %1", QString::fromUtf8(zip_strerror(archive))));
            return false;
        }
    }
    if (!password().isEmpty()) {
        Q_ASSERT(!options.encryptionMethod().isEmpty());
        if (options.encryptionMethod() == QLatin1String("AES128")) {
            zip_file_set_encryption(archive, index, ZIP_EM_AES_128, password().toUtf8());
        } else if (options.encryptionMethod() == QLatin1String("AES192")) {
            zip_file_set_encryption(archive, index, ZIP_EM_AES_192, password().toUtf8());
        } else if (options.encryptionMethod() == QLatin1String("AES256")) {
            zip_file_set_encryption(archive, index, ZIP_EM_AES_256, password().toUtf8());
        }
    }

    return true;
}

bool LibzipPlugin::emitEntryForIndex(zip_t *archive, qlonglong index)
{
    Q_ASSERT(archive);

    zip_stat_t sb;
    if (zip_stat_index(archive, index, ZIP_FL_ENC_GUESS, &sb)) {
        qCCritical(ARK) << "Failed to read stat for index" << index;
        return false;
    }

    auto e = new Archive::Entry();

    if (sb.valid & ZIP_STAT_NAME) {
        e->setFullPath(QString::fromUtf8(sb.name));
    }

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
            case ZIP_CM_XZ:
                e->setProperty("method", QStringLiteral("XZ"));
                emit compressionMethodFound(QStringLiteral("XZ"));
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
    m_emittedEntries << e;

    return true;
}

bool LibzipPlugin::deleteFiles(const QVector<Archive::Entry*> &files)
{
    int errcode;
    zip_error_t err;

    // Open archive.
    zip_t *archive = zip_open(QFile::encodeName(filename()), 0, &errcode);
    zip_error_init_with_code(&err, errcode);
    if (archive == nullptr) {
        qCCritical(ARK) << "Failed to open archive. Code:" << errcode;
        emit error(xi18n("Failed to open archive: %1", QString::fromUtf8(zip_error_strerror(&err))));
        return false;
    }

    qulonglong i = 0;
    foreach (const Archive::Entry* e, files) {

        if (QThread::currentThread()->isInterruptionRequested()) {
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

    if (zip_close(archive)) {
        qCCritical(ARK) << "Failed to write archive";
        emit error(xi18n("Failed to write archive."));
        return false;
    }
    return true;
}

bool LibzipPlugin::addComment(const QString& comment)
{
    int errcode;
    zip_error_t err;

    // Open archive.
    zip_t *archive = zip_open(QFile::encodeName(filename()), 0, &errcode);
    zip_error_init_with_code(&err, errcode);
    if (archive == nullptr) {
        qCCritical(ARK) << "Failed to open archive. Code:" << errcode;
        emit error(xi18n("Failed to open archive: %1", QString::fromUtf8(zip_error_strerror(&err))));
        return false;
    }

    // Set archive comment.
    if (zip_set_archive_comment(archive, comment.toUtf8(), comment.length())) {
        qCCritical(ARK) << "Failed to set comment:" << zip_strerror(archive);
        return false;
    }

    if (zip_close(archive)) {
        qCCritical(ARK) << "Failed to write archive";
        emit error(xi18n("Failed to write archive."));
        return false;
    }
    return true;
}

bool LibzipPlugin::testArchive()
{
    qCDebug(ARK) << "Testing archive";

    int errcode;
    zip_error_t err;

    // Open archive performing extra consistency checks.
    zip_t *archive = zip_open(QFile::encodeName(filename()), ZIP_CHECKCONS, &errcode);
    zip_error_init_with_code(&err, errcode);
    if (archive == nullptr) {
        qCCritical(ARK) << "Failed to open archive:" << zip_error_strerror(&err);
        return false;
    }
    zip_close(archive);

    emit testSuccess();
    return true;
}

bool LibzipPlugin::doKill()
{
    qCDebug(ARK) << "Killing operation...";
    return true;
}

bool LibzipPlugin::extractFiles(const QVector<Archive::Entry*> &files, const QString& destinationDirectory, const ExtractionOptions& options)
{
    qCDebug(ARK) << "Extracting files to:" << destinationDirectory;

    const bool extractAll = files.isEmpty();
    const bool removeRootNode = options.isDragAndDropEnabled();

    int errcode;
    zip_error_t err;

    // Open archive.
    zip_t *archive = zip_open(QFile::encodeName(filename()), ZIP_RDONLY, &errcode);
    zip_error_init_with_code(&err, errcode);
    if (archive == nullptr) {
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
            if (QThread::currentThread()->isInterruptionRequested()) {
                break;
            }
            if (!extractEntry(archive,
                              QDir::fromNativeSeparators(QString::fromUtf8(zip_get_name(archive, i, ZIP_FL_ENC_GUESS))),
                              QString(),
                              destinationDirectory,
                              options.preservePaths(),
                              removeRootNode)) {
                qCDebug(ARK) << "Extraction failed";
                return false;
            }
            emit progress(float(i + 1) / nofEntries);
        }
    } else {
        // We extract only the entries in files.
        qulonglong i = 0;
        foreach (const Archive::Entry* e, files) {
            if (QThread::currentThread()->isInterruptionRequested()) {
                break;
            }
            if (!extractEntry(archive,
                              e->fullPath(),
                              e->rootNode,
                              destinationDirectory,
                              options.preservePaths(),
                              removeRootNode)) {
                qCDebug(ARK) << "Extraction failed";
                return false;
            }
            emit progress(float(++i) / nofEntries);
        }
    }

    zip_close(archive);
    return true;
}

bool LibzipPlugin::extractEntry(zip_t *archive, const QString &entry, const QString &rootNode, const QString &destDir, bool preservePaths, bool removeRootNode)
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
        if (!removeRootNode || rootNode.isEmpty()) {
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

    // Store parent mtime.
    QString parentDir;
    if (isDirectory) {
        QDir pDir = QFileInfo(destination).dir();
        pDir.cdUp();
        parentDir = pDir.path();
    } else {
        parentDir = QFileInfo(destination).path();
    }
    // For top-level items, dont restore parent dir mtime.
    bool restoreParentMtime = (parentDir + QDir::separator() != destDirCorrected);

    time_t parent_mtime;
    if (restoreParentMtime) {
        parent_mtime = QFileInfo(parentDir).lastModified().toMSecsSinceEpoch() / 1000;
    }

    // Create parent directories for files. For directories create them.
    if (!QDir().mkpath(QFileInfo(destination).path())) {
        qCDebug(ARK) << "Failed to create directory:" << QFileInfo(destination).path();
        emit error(xi18n("Failed to create directory: %1", QFileInfo(destination).path()));
        return false;
    }

    // Get statistic for entry. Used to get entry size and mtime.
    zip_stat_t sb;
    if (zip_stat(archive, entry.toUtf8(), 0, &sb) != 0) {
        qCCritical(ARK) << "Failed to read stat for entry" << entry;
        return false;
    }

    if (!isDirectory) {

        // Handle existing destination files.
        QString renamedEntry = entry;
        while (!m_overwriteAll && QFileInfo::exists(destination)) {
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
        zip_file *zf = nullptr;
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
                qCCritical(ARK) << "Failed to open file:" << zip_strerror(archive);
                emit error(xi18n("Failed to open '%1':<nl/>%2", entry, QString::fromUtf8(zip_strerror(archive))));
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

        const auto index = zip_name_locate(archive, entry.toUtf8(), ZIP_FL_ENC_GUESS);
        if (index == -1) {
            qCCritical(ARK) << "Could not locate entry:" << entry;
            emit error(xi18n("Failed to locate entry: %1", entry));
            return false;
        }

        zip_uint8_t opsys;
        zip_uint32_t attributes;
        if (zip_file_get_external_attributes(archive, index, ZIP_FL_UNCHANGED, &opsys, &attributes) == -1) {
            qCCritical(ARK) << "Could not read external attributes for entry:" << entry;
            emit error(xi18n("Failed to read metadata for entry: %1", entry));
            return false;
        }

        // Inspired by fuse-zip source code: fuse-zip/lib/fileNode.cpp
        switch (opsys) {
        case ZIP_OPSYS_UNIX:
            // Unix permissions are stored in the leftmost 16 bits of the external file attribute.
            file.setPermissions(KIO::convertPermissions(attributes >> 16));
            break;
        default:    // TODO: non-UNIX.
            break;
        }

        file.close();
    }

    // Set mtime for entry.
    utimbuf times;
    times.modtime = sb.mtime;
    if (utime(destination.toUtf8(), &times) != 0) {
        qCWarning(ARK) << "Failed to restore mtime:" << destination;
    }

    if (restoreParentMtime) {
        // Restore mtime for parent dir.
        times.modtime = parent_mtime;
        if (utime(parentDir.toUtf8(), &times) != 0) {
            qCWarning(ARK) << "Failed to restore mtime for parent dir of:" << destination;
        }
    }

    return true;
}

bool LibzipPlugin::moveFiles(const QVector<Archive::Entry*> &files, Archive::Entry *destination, const CompressionOptions &options)
{
    Q_UNUSED(options)

    int errcode;
    zip_error_t err;

    // Open archive.
    zip_t *archive = zip_open(QFile::encodeName(filename()), 0, &errcode);
    zip_error_init_with_code(&err, errcode);
    if (archive == nullptr) {
        qCCritical(ARK) << "Failed to open archive. Code:" << errcode;
        emit error(xi18n("Failed to open archive: %1", QString::fromUtf8(zip_error_strerror(&err))));
        return false;
    }

    QStringList filePaths = entryFullPaths(files);
    filePaths.sort();
    const QStringList destPaths = entryPathsFromDestination(filePaths, destination, entriesWithoutChildren(files).count());

    int i;
    for (i = 0; i < filePaths.size(); ++i) {

        const int index = zip_name_locate(archive, filePaths.at(i).toUtf8(), ZIP_FL_ENC_GUESS);
        if (index == -1) {
            qCCritical(ARK) << "Could not find entry to move:" << filePaths.at(i);
            emit error(xi18n("Failed to move entry: %1", filePaths.at(i)));
            return false;
        }

        if (zip_file_rename(archive, index, destPaths.at(i).toUtf8(), ZIP_FL_ENC_GUESS) == -1) {
            qCCritical(ARK) << "Could not move entry:" << filePaths.at(i);
            emit error(xi18n("Failed to move entry: %1", filePaths.at(i)));
            return false;
        }

        emit entryRemoved(filePaths.at(i));
        emitEntryForIndex(archive, index);
        emit progress(i/filePaths.count());
    }
    if (zip_close(archive)) {
        qCCritical(ARK) << "Failed to write archive";
        emit error(xi18n("Failed to write archive."));
        return false;
    }
    qCDebug(ARK) << "Moved" << i << "entries";

    return true;
}

bool LibzipPlugin::copyFiles(const QVector<Archive::Entry*> &files, Archive::Entry *destination, const CompressionOptions &options)
{
    Q_UNUSED(options)

    int errcode;
    zip_error_t err;

    // Open archive.
    zip_t *archive = zip_open(QFile::encodeName(filename()), 0, &errcode);
    zip_error_init_with_code(&err, errcode);
    if (archive == nullptr) {
        qCCritical(ARK) << "Failed to open archive. Code:" << errcode;
        emit error(xi18n("Failed to open archive: %1", QString::fromUtf8(zip_error_strerror(&err))));
        return false;
    }

    const QStringList filePaths = entryFullPaths(files);
    const QStringList destPaths = entryPathsFromDestination(filePaths, destination, 0);

    int i;
    for (i = 0; i < filePaths.size(); ++i) {

        QString dest = destPaths.at(i);

        if (dest.endsWith(QDir::separator())) {
            if (zip_dir_add(archive, dest.toUtf8(), ZIP_FL_ENC_GUESS) == -1) {
                // If directory already exists in archive, we get an error.
                qCWarning(ARK) << "Failed to add dir " << dest << ":" << zip_strerror(archive);
                continue;
            }
        }

        const int srcIndex = zip_name_locate(archive, filePaths.at(i).toUtf8(), ZIP_FL_ENC_GUESS);
        if (srcIndex == -1) {
            qCCritical(ARK) << "Could not find entry to copy:" << filePaths.at(i);
            emit error(xi18n("Failed to copy entry: %1", filePaths.at(i)));
            return false;
        }

        zip_source_t *src = zip_source_zip(archive, archive, srcIndex, 0, 0, -1);
        if (!src) {
            qCCritical(ARK) << "Failed to create source for:" << filePaths.at(i);
            return false;
        }

        const int destIndex = zip_file_add(archive, dest.toUtf8(), src, ZIP_FL_ENC_GUESS | ZIP_FL_OVERWRITE);
        if (destIndex == -1) {
            zip_source_free(src);
            qCCritical(ARK) << "Could not add entry" << dest << ":" << zip_strerror(archive);
            emit error(xi18n("Failed to add entry: %1", QString::fromUtf8(zip_strerror(archive))));
            return false;
        }

        emitEntryForIndex(archive, destIndex);
        emit progress(i/filePaths.count());
    }
    if (zip_close(archive)) {
        qCCritical(ARK) << "Failed to write archive";
        emit error(xi18n("Failed to write archive."));
        return false;
    }
    qCDebug(ARK) << "Copied" << i << "entries";

    return true;
}

#include "libzipplugin.moc"
