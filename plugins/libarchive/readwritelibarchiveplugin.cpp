/*
 * Copyright (c) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (c) 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (c) 2010 Raphael Kubo da Costa <rakuco@FreeBSD.org>
 * Copyright (c) 2016 Vladyslav Batyrenko <mvlabat@gmail.com>
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

#include "readwritelibarchiveplugin.h"
#include "ark_debug.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QDirIterator>
#include <QSaveFile>
#include <QThread>

#include <archive_entry.h>

K_PLUGIN_CLASS_WITH_JSON(ReadWriteLibarchivePlugin, "kerfuffle_libarchive.json")

ReadWriteLibarchivePlugin::ReadWriteLibarchivePlugin(QObject *parent, const QVariantList &args)
    : LibarchivePlugin(parent, args)
{
    qCDebug(ARK) << "Loaded libarchive read-write plugin";
}

ReadWriteLibarchivePlugin::~ReadWriteLibarchivePlugin()
{
}

bool ReadWriteLibarchivePlugin::addFiles(const QVector<Archive::Entry*> &files, const Archive::Entry *destination, const CompressionOptions &options, uint numberOfEntriesToAdd)
{
    qCDebug(ARK) << "Adding" << files.size() << "entries with CompressionOptions" << options;

    const bool creatingNewFile = !QFileInfo::exists(filename());
    const uint totalCount = m_numberOfEntries + numberOfEntriesToAdd;

    m_writtenFiles.clear();

    if (!creatingNewFile && !initializeReader()) {
        return false;
    }

    if (!initializeWriter(creatingNewFile, options)) {
        return false;
    }

    // First write the new files.
    qCDebug(ARK) << "Writing new entries";
    uint addedEntries = 0;
    // Recreate destination directory structure.
    const QString destinationPath = (destination == nullptr)
                                    ? QString()
                                    : destination->fullPath();

    for (Archive::Entry *selectedFile : files) {
        if (QThread::currentThread()->isInterruptionRequested()) {
            break;
        }

        if (!writeFile(selectedFile->fullPath(), destinationPath)) {
            finish(false);
            return false;
        }
        addedEntries++;
        emit progress(float(addedEntries)/float(totalCount));

        // For directories, write all subfiles/folders.
        const QString &fullPath = selectedFile->fullPath();
        if (QFileInfo(fullPath).isDir()) {
            QDirIterator it(fullPath,
                            QDir::AllEntries | QDir::Readable |
                            QDir::Hidden | QDir::NoDotAndDotDot,
                            QDirIterator::Subdirectories);

            while (!QThread::currentThread()->isInterruptionRequested() && it.hasNext()) {
                QString path = it.next();

                if ((it.fileName() == QLatin1String("..")) ||
                    (it.fileName() == QLatin1String("."))) {
                    continue;
                }

                const bool isRealDir = it.fileInfo().isDir() && !it.fileInfo().isSymLink();

                if (isRealDir) {
                    path.append(QLatin1Char('/'));
                }

                if (!writeFile(path, destinationPath)) {
                    finish(false);
                    return false;
                }
                addedEntries++;
                emit progress(float(addedEntries)/float(totalCount));
            }
        }
    }
    qCDebug(ARK) << "Added" << addedEntries << "new entries to archive";

    bool isSuccessful = true;
    // If we have old archive entries.
    if (!creatingNewFile) {
        qCDebug(ARK) << "Copying any old entries";
        m_filesPaths = m_writtenFiles;
        isSuccessful = processOldEntries(addedEntries, Add, totalCount);
        if (isSuccessful) {
            qCDebug(ARK) << "Added" << addedEntries << "old entries to archive";
        } else {
            qCDebug(ARK) << "Adding entries failed";
        }
    }

    finish(isSuccessful);
    return isSuccessful;
}

bool ReadWriteLibarchivePlugin::moveFiles(const QVector<Archive::Entry*> &files, Archive::Entry *destination, const CompressionOptions &options)
{
    Q_UNUSED(options);

    qCDebug(ARK) << "Moving" << files.size() << "entries";

    if (!initializeReader()) {
        return false;
    }

    if (!initializeWriter()) {
        return false;
    }

    // Copy old elements from previous archive to new archive.
    uint movedEntries = 0;
    m_filesPaths = entryFullPaths(files);
    m_entriesWithoutChildren = entriesWithoutChildren(files).count();
    m_destination = destination;
    const bool isSuccessful = processOldEntries(movedEntries, Move, m_numberOfEntries);
    if (isSuccessful) {
        qCDebug(ARK) << "Moved" << movedEntries << "entries within archive";
    } else {
        qCDebug(ARK) << "Moving entries failed";
    }

    finish(isSuccessful);
    return isSuccessful;
}

bool ReadWriteLibarchivePlugin::copyFiles(const QVector<Archive::Entry*> &files, Archive::Entry *destination, const CompressionOptions &options)
{
    Q_UNUSED(options);

    qCDebug(ARK) << "Copying" << files.size() << "entries";

    if (!initializeReader()) {
        return false;
    }

    if (!initializeWriter()) {
        return false;
    }

    // Copy old elements from previous archive to new archive.
    uint copiedEntries = 0;
    m_filesPaths = entryFullPaths(files);
    m_destination = destination;
    const bool isSuccessful = processOldEntries(copiedEntries, Copy, m_numberOfEntries);
    if (isSuccessful) {
        qCDebug(ARK) << "Copied" << copiedEntries << "entries within archive";
    } else {
        qCDebug(ARK) << "Copying entries failed";
    }

    finish(isSuccessful);
    return isSuccessful;
}

bool ReadWriteLibarchivePlugin::deleteFiles(const QVector<Archive::Entry*> &files)
{
    qCDebug(ARK) << "Deleting" << files.size() << "entries";

    if (!initializeReader()) {
        return false;
    }

    if (!initializeWriter()) {
        return false;
    }

    // Copy old elements from previous archive to new archive.
    uint deletedEntries = 0;
    m_filesPaths = entryFullPaths(files);
    const bool isSuccessful = processOldEntries(deletedEntries, Delete, m_numberOfEntries);
    if (isSuccessful) {
        qCDebug(ARK) << "Removed" << deletedEntries << "entries from archive";
    } else {
        qCDebug(ARK) << "Removing entries failed";
    }

    finish(isSuccessful);
    return isSuccessful;
}

bool ReadWriteLibarchivePlugin::initializeWriter(const bool creatingNewFile, const CompressionOptions &options)
{
    m_tempFile.setFileName(filename());
    if (!m_tempFile.open(QIODevice::WriteOnly | QIODevice::Unbuffered)) {
        emit error(i18nc("@info", "Failed to create a temporary file for writing data."));
        return false;
    }

    m_archiveWriter.reset(archive_write_new());
    if (!(m_archiveWriter.data())) {
        emit error(i18n("The archive writer could not be initialized."));
        return false;
    }

    // pax_restricted is the libarchive default, let's go with that.
    archive_write_set_format_pax_restricted(m_archiveWriter.data());

    if (creatingNewFile) {
        if (!initializeNewFileWriterFilters(options)) {
            return false;
        }
    } else {
        if (!initializeWriterFilters()) {
            return false;
        }
    }

    if (archive_write_open_fd(m_archiveWriter.data(), m_tempFile.handle()) != ARCHIVE_OK) {
        emit error(i18nc("@info", "Could not open the archive for writing entries."));
        return false;
    }

    return true;
}

bool ReadWriteLibarchivePlugin::initializeWriterFilters()
{
    int ret;
    bool requiresExecutable = false;
    switch (archive_filter_code(m_archiveReader.data(), 0)) {
    case ARCHIVE_FILTER_GZIP:
        ret = archive_write_add_filter_gzip(m_archiveWriter.data());
        break;
    case ARCHIVE_FILTER_BZIP2:
        ret = archive_write_add_filter_bzip2(m_archiveWriter.data());
        break;
    case ARCHIVE_FILTER_XZ:
        ret = archive_write_add_filter_xz(m_archiveWriter.data());
        break;
    case ARCHIVE_FILTER_LZMA:
        ret = archive_write_add_filter_lzma(m_archiveWriter.data());
        break;
    case ARCHIVE_FILTER_COMPRESS:
        ret = archive_write_add_filter_compress(m_archiveWriter.data());
        break;
    case ARCHIVE_FILTER_LZIP:
        ret = archive_write_add_filter_lzip(m_archiveWriter.data());
        break;
    case ARCHIVE_FILTER_LZOP:
        ret = archive_write_add_filter_lzop(m_archiveWriter.data());
        break;
    case ARCHIVE_FILTER_LRZIP:
        ret = archive_write_add_filter_lrzip(m_archiveWriter.data());
        requiresExecutable = true;
        break;
    case ARCHIVE_FILTER_LZ4:
        ret = archive_write_add_filter_lz4(m_archiveWriter.data());
        break;
#ifdef HAVE_ZSTD_SUPPORT
    case ARCHIVE_FILTER_ZSTD:
        ret = archive_write_add_filter_zstd(m_archiveWriter.data());
        break;
#endif
#ifdef HAVE_BROTLI_SUPPORT
    case ARCHIVE_FILTER_BROTLI:
        ret = archive_write_add_filter_brotli(m_archiveWriter.data());
        break;
#endif
    case ARCHIVE_FILTER_NONE:
        ret = archive_write_add_filter_none(m_archiveWriter.data());
        break;
    default:
        emit error(i18n("The compression type '%1' is not supported by Ark.",
                        QLatin1String(archive_filter_name(m_archiveReader.data(), 0))));
        return false;
    }

    // Libarchive emits a warning for lrzip due to using external executable.
    if ((requiresExecutable && ret != ARCHIVE_WARN) ||
        (!requiresExecutable && ret != ARCHIVE_OK)) {
        qCWarning(ARK) << "Failed to set compression method:" << archive_error_string(m_archiveWriter.data());
        emit error(i18nc("@info", "Could not set the compression method."));
        return false;
    }

    return true;
}

bool ReadWriteLibarchivePlugin::initializeNewFileWriterFilters(const CompressionOptions &options)
{
    int ret;
    bool requiresExecutable = false;
    if (filename().right(2).toUpper() == QLatin1String("GZ")) {
        qCDebug(ARK) << "Detected gzip compression for new file";
        ret = archive_write_add_filter_gzip(m_archiveWriter.data());
    } else if (filename().right(3).toUpper() == QLatin1String("BZ2")) {
        qCDebug(ARK) << "Detected bzip2 compression for new file";
        ret = archive_write_add_filter_bzip2(m_archiveWriter.data());
    } else if (filename().right(2).toUpper() == QLatin1String("XZ")) {
        qCDebug(ARK) << "Detected xz compression for new file";
        ret = archive_write_add_filter_xz(m_archiveWriter.data());
    } else if (filename().right(4).toUpper() == QLatin1String("LZMA")) {
        qCDebug(ARK) << "Detected lzma compression for new file";
        ret = archive_write_add_filter_lzma(m_archiveWriter.data());
    } else if (filename().right(2).toUpper() == QLatin1String(".Z")) {
        qCDebug(ARK) << "Detected compress (.Z) compression for new file";
        ret = archive_write_add_filter_compress(m_archiveWriter.data());
    } else if (filename().right(2).toUpper() == QLatin1String("LZ")) {
        qCDebug(ARK) << "Detected lzip compression for new file";
        ret = archive_write_add_filter_lzip(m_archiveWriter.data());
    } else if (filename().right(3).toUpper() == QLatin1String("LZO")) {
        qCDebug(ARK) << "Detected lzop compression for new file";
        ret = archive_write_add_filter_lzop(m_archiveWriter.data());
    } else if (filename().right(3).toUpper() == QLatin1String("LRZ")) {
        qCDebug(ARK) << "Detected lrzip compression for new file";
        ret = archive_write_add_filter_lrzip(m_archiveWriter.data());
        requiresExecutable = true;
    } else if (filename().right(3).toUpper() == QLatin1String("LZ4")) {
        qCDebug(ARK) << "Detected lz4 compression for new file";
        ret = archive_write_add_filter_lz4(m_archiveWriter.data());
#ifdef HAVE_ZSTD_SUPPORT
    } else if (filename().right(3).toUpper() == QLatin1String("ZST")) {
        qCDebug(ARK) << "Detected zstd compression for new file";
        ret = archive_write_add_filter_zstd(m_archiveWriter.data());
#endif
#ifdef HAVE_BROTLI_SUPPORT
    } else if (filename().right(2).toUpper() == QLatin1String("BR")) {
        qCDebug(ARK) << "Detected Brotli compression for new file";
        ret = archive_write_add_filter_brotli(m_archiveWriter.data());
#endif
    } else if (filename().right(3).toUpper() == QLatin1String("TAR")) {
        qCDebug(ARK) << "Detected no compression for new file (pure tar)";
        ret = archive_write_add_filter_none(m_archiveWriter.data());
    } else {
        qCDebug(ARK) << "Falling back to gzip";
        ret = archive_write_add_filter_gzip(m_archiveWriter.data());
    }

    // Libarchive emits a warning for lrzip due to using external executable.
    if ((requiresExecutable && ret != ARCHIVE_WARN) ||
        (!requiresExecutable && ret != ARCHIVE_OK)) {
        qCWarning(ARK) << "Failed to set compression method:" << archive_error_string(m_archiveWriter.data());
        emit error(i18nc("@info", "Could not set the compression method."));
        return false;
    }

    // Set compression level if passed in CompressionOptions.
    if (options.isCompressionLevelSet()) {
        qCDebug(ARK) << "Using compression level:" << options.compressionLevel();
        ret = archive_write_set_filter_option(m_archiveWriter.data(), nullptr, "compression-level", QString::number(options.compressionLevel()).toUtf8().constData());
        if (ret != ARCHIVE_OK) {
            qCWarning(ARK) << "Failed to set compression level" << archive_error_string(m_archiveWriter.data());
            emit error(i18nc("@info", "Could not set the compression level."));
            return false;
        }
    }

    return true;
}

void ReadWriteLibarchivePlugin::finish(const bool isSuccessful)
{
    if (!isSuccessful || QThread::currentThread()->isInterruptionRequested()) {
        archive_write_fail(m_archiveWriter.data());
        m_tempFile.cancelWriting();
    } else {
        // archive_write_close() needs to be called before calling QSaveFile::commit(),
        // otherwise the latter will close() the file descriptor m_archiveWriter is still working on.
        // TODO: We need to abstract this code better so that we only deal with one
        // object that manages both QSaveFile and ArchiveWriter.
        archive_write_close(m_archiveWriter.data());
        m_tempFile.commit();
    }
}

bool ReadWriteLibarchivePlugin::processOldEntries(uint &entriesCounter, OperationMode mode, uint totalCount)
{
    const uint newEntries = entriesCounter;
    entriesCounter = 0;
    uint iteratedEntries = 0;

    // Create a map that contains old path as key and new path as value.
    QMap<QString, QString> pathMap;
    if (mode == Move || mode == Copy) {
        m_filesPaths.sort();
        QStringList resultList = entryPathsFromDestination(m_filesPaths, m_destination, m_entriesWithoutChildren);
        const int listSize = m_filesPaths.count();
        Q_ASSERT(listSize == resultList.count());
        for (int i = 0; i < listSize; ++i) {
            pathMap.insert(m_filesPaths.at(i), resultList.at(i));
        }
    }

    struct archive_entry *entry;
    while (!QThread::currentThread()->isInterruptionRequested() && archive_read_next_header(m_archiveReader.data(), &entry) == ARCHIVE_OK) {

        const QString file = QFile::decodeName(archive_entry_pathname(entry));

        if (mode == Move || mode == Copy) {
            const QString newPathname = pathMap.value(file);
            if (!newPathname.isEmpty()) {
                if (mode == Copy) {
                    // Write the old entry.
                    if (!writeEntry(entry)) {
                        return false;
                    }
                } else {
                    emit entryRemoved(file);
                }

                entriesCounter++;
                iteratedEntries--;

                // Change entry path.
                archive_entry_set_pathname(entry, newPathname.toUtf8().constData());
                emitEntryFromArchiveEntry(entry);
            }
        } else if (m_filesPaths.contains(file)) {
            archive_read_data_skip(m_archiveReader.data());
            switch (mode) {
            case Delete:
                entriesCounter++;
                emit entryRemoved(file);
                emit progress(float(newEntries + entriesCounter + iteratedEntries)/float(totalCount));
                break;

            case Add:
                qCDebug(ARK) << file << "is already present in the new archive, skipping.";
                // When overwriting entries, we need to decrement the counter manually,
                // because entry was emitted.
                m_numberOfEntries--;
                break;

            default:
                qCDebug(ARK) << "Mode" << mode << "is not considered for processing old libarchive entries";
                Q_ASSERT(false);
            }
            continue;
        }

        // Write old entries.
        if (writeEntry(entry)) {
            if (mode == Add) {
                entriesCounter++;
            } else if (mode == Move || mode == Copy) {
                iteratedEntries++;
            } else if (mode == Delete) {
                iteratedEntries++;
            }
        } else {
            return false;
        }
        emit progress(float(newEntries + entriesCounter + iteratedEntries)/float(totalCount));
    }

    return !QThread::currentThread()->isInterruptionRequested();
}

bool ReadWriteLibarchivePlugin::writeEntry(struct archive_entry *entry)
{
    const int returnCode = archive_write_header(m_archiveWriter.data(), entry);
    switch (returnCode) {
    case ARCHIVE_OK:
        // If the whole archive is extracted and the total filesize is
        // available, we use partial progress.
        copyData(QLatin1String(archive_entry_pathname(entry)), m_archiveReader.data(), m_archiveWriter.data(), false);
        break;
    case ARCHIVE_FAILED:
    case ARCHIVE_FATAL:
        qCCritical(ARK) << "archive_write_header() has returned" << returnCode
                        << "with errno" << archive_errno(m_archiveWriter.data());
        emit error(i18nc("@info", "Could not compress entry, operation aborted."));
        return false;
    default:
        qCDebug(ARK) << "archive_writer_header() has returned" << returnCode
                     << "which will be ignored.";
        break;
    }

    return true;
}

// TODO: if we merge this with copyData(), we can pass more data
//       such as an fd to archive_read_disk_entry_from_file()
bool ReadWriteLibarchivePlugin::writeFile(const QString &relativeName, const QString &destination)
{
    const QString absoluteFilename = QFileInfo(relativeName).absoluteFilePath();
    const QString destinationFilename = destination + relativeName;

    // #253059: Even if we use archive_read_disk_entry_from_file,
    //          libarchive may have been compiled without HAVE_LSTAT,
    //          or something may have caused it to follow symlinks, in
    //          which case stat() will be called. To avoid this, we
    //          call lstat() ourselves.
    struct stat st;
    lstat(QFile::encodeName(absoluteFilename).constData(), &st); // krazy:exclude=syscalls

    struct archive_entry *entry = archive_entry_new();
    archive_entry_set_pathname(entry, QFile::encodeName(destinationFilename).constData());
    archive_entry_copy_sourcepath(entry, QFile::encodeName(absoluteFilename).constData());
    archive_read_disk_entry_from_file(m_archiveReadDisk.data(), entry, -1, &st);

    const auto returnCode = archive_write_header(m_archiveWriter.data(), entry);
    if (returnCode == ARCHIVE_OK) {
        // If the whole archive is extracted and the total filesize is
        // available, we use partial progress.
        copyData(absoluteFilename, m_archiveWriter.data(), false);
    } else {
        qCCritical(ARK) << "Writing header failed with error code " << returnCode;
        qCCritical(ARK) << "Error while writing..." << archive_error_string(m_archiveWriter.data()) << "(error no =" << archive_errno(m_archiveWriter.data()) << ')';

        emit error(i18nc("@info Error in a message box",
                         "Could not compress entry."));

        archive_entry_free(entry);

        return false;
    }

    if (QThread::currentThread()->isInterruptionRequested()) {
        archive_entry_free(entry);
        return false;
    }

    m_writtenFiles.push_back(destinationFilename);

    emitEntryFromArchiveEntry(entry);

    archive_entry_free(entry);

    return true;
}

#include "readwritelibarchiveplugin.moc"
