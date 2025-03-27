/*
    SPDX-FileCopyrightText: 2007 Henrique Pinto <henrique.pinto@kdemail.net>
    SPDX-FileCopyrightText: 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
    SPDX-FileCopyrightText: 2010 Raphael Kubo da Costa <rakuco@FreeBSD.org>
    SPDX-FileCopyrightText: 2016 Vladyslav Batyrenko <mvlabat@gmail.com>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "readwritelibarchiveplugin.h"
#include "ark_debug.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QDirIterator>
#include <QThread>

#include <archive_entry.h>

K_PLUGIN_CLASS_WITH_JSON(ReadWriteLibarchivePlugin, "kerfuffle_libarchive.json")

ReadWriteLibarchivePlugin::ReadWriteLibarchivePlugin(QObject *parent, const QVariantList &args)
    : LibarchivePlugin(parent, args)
{
    qCDebug(ARK_LOG) << "Loaded libarchive read-write plugin";
}

ReadWriteLibarchivePlugin::~ReadWriteLibarchivePlugin()
{
}

bool ReadWriteLibarchivePlugin::addFiles(const QList<Archive::Entry *> &files,
                                         const Archive::Entry *destination,
                                         const CompressionOptions &options,
                                         uint numberOfEntriesToAdd)
{
    qCDebug(ARK_LOG) << "Adding" << files.size() << "entries with CompressionOptions" << options;

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
    qCDebug(ARK_LOG) << "Writing new entries";
    uint addedEntries = 0;
    // Recreate destination directory structure.
    const QString destinationPath = (destination == nullptr) ? QString() : destination->fullPath();

    for (Archive::Entry *selectedFile : files) {
        if (QThread::currentThread()->isInterruptionRequested()) {
            break;
        }

        if (!writeFile(selectedFile->fullPath(), destinationPath)) {
            finish(false);
            return false;
        }
        addedEntries++;
        Q_EMIT progress(float(addedEntries) / float(totalCount));

        // For directories, write all subfiles/folders.
        const QString &fullPath = selectedFile->fullPath();
        if (QFileInfo(fullPath).isDir()) {
            QDirIterator it(fullPath, QDir::AllEntries | QDir::Readable | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

            while (!QThread::currentThread()->isInterruptionRequested() && it.hasNext()) {
                QString path = it.next();

                if ((it.fileName() == QLatin1String("..")) || (it.fileName() == QLatin1Char('.'))) {
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
                Q_EMIT progress(float(addedEntries) / float(totalCount));
            }
        }
    }
    qCDebug(ARK_LOG) << "Added" << addedEntries << "new entries to archive";

    bool isSuccessful = true;
    // If we have old archive entries.
    if (!creatingNewFile) {
        qCDebug(ARK_LOG) << "Copying any old entries";
        m_filesPaths = m_writtenFiles;
        isSuccessful = processOldEntries(addedEntries, Add, totalCount);
        if (isSuccessful) {
            qCDebug(ARK_LOG) << "Added" << addedEntries << "old entries to archive";
        } else {
            qCDebug(ARK_LOG) << "Adding entries failed";
        }
    }

    finish(isSuccessful);
    return isSuccessful;
}

bool ReadWriteLibarchivePlugin::moveFiles(const QList<Archive::Entry *> &files, Archive::Entry *destination, const CompressionOptions &options)
{
    Q_UNUSED(options);

    qCDebug(ARK_LOG) << "Moving" << files.size() << "entries";

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
        qCDebug(ARK_LOG) << "Moved" << movedEntries << "entries within archive";
    } else {
        qCDebug(ARK_LOG) << "Moving entries failed";
    }

    finish(isSuccessful);
    return isSuccessful;
}

bool ReadWriteLibarchivePlugin::copyFiles(const QList<Archive::Entry *> &files, Archive::Entry *destination, const CompressionOptions &options)
{
    Q_UNUSED(options);

    qCDebug(ARK_LOG) << "Copying" << files.size() << "entries";

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
        qCDebug(ARK_LOG) << "Copied" << copiedEntries << "entries within archive";
    } else {
        qCDebug(ARK_LOG) << "Copying entries failed";
    }

    finish(isSuccessful);
    return isSuccessful;
}

bool ReadWriteLibarchivePlugin::deleteFiles(const QList<Archive::Entry *> &files)
{
    qCDebug(ARK_LOG) << "Deleting" << files.size() << "entries";

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
        qCDebug(ARK_LOG) << "Removed" << deletedEntries << "entries from archive";
    } else {
        qCDebug(ARK_LOG) << "Removing entries failed";
    }

    finish(isSuccessful);
    return isSuccessful;
}

void ReadWriteLibarchivePlugin::initializeWriterFormat()
{
    if (filename().endsWith(QLatin1String("7z"), Qt::CaseInsensitive)) {
        archive_write_set_format_7zip(m_archiveWriter.data());
    } else {
        // TAR case:
        // pax_restricted is the libarchive default, let's go with that.
        archive_write_set_format_pax_restricted(m_archiveWriter.data());
    }
}

bool ReadWriteLibarchivePlugin::initializeWriter(const bool creatingNewFile, const CompressionOptions &options)
{
    m_tempFile.setFileName(filename());
    if (!m_tempFile.open(QIODevice::WriteOnly | QIODevice::Unbuffered)) {
        Q_EMIT error(i18nc("@info", "Failed to create a temporary file for writing data."));
        return false;
    }

    m_archiveWriter.reset(archive_write_new());
    if (!(m_archiveWriter.data())) {
        Q_EMIT error(i18n("The archive writer could not be initialized."));
        return false;
    }

    initializeWriterFormat();

    if (creatingNewFile) {
        if (!initializeNewFileCompressionOptions(options)) {
            return false;
        }
    } else {
        if (!initializeWriterFilters()) {
            return false;
        }
    }

    if (archive_write_open_fd(m_archiveWriter.data(), m_tempFile.handle()) != ARCHIVE_OK) {
        Q_EMIT error(i18nc("@info", "Could not open the archive for writing entries."));
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
    case ARCHIVE_FILTER_ZSTD:
        ret = archive_write_add_filter_zstd(m_archiveWriter.data());
        break;
    case ARCHIVE_FILTER_NONE:
        ret = archive_write_add_filter_none(m_archiveWriter.data());
        break;
    default:
        Q_EMIT error(i18n("The compression type '%1' is not supported by Ark.", QLatin1String(archive_filter_name(m_archiveReader.data(), 0))));
        return false;
    }

    // Libarchive emits a warning for lrzip due to using external executable.
    if ((requiresExecutable && ret != ARCHIVE_WARN) || (!requiresExecutable && ret != ARCHIVE_OK)) {
        qCWarning(ARK_LOG) << "Failed to set compression method:" << archive_error_string(m_archiveWriter.data());
        Q_EMIT error(i18nc("@info", "Could not set the compression method."));
        return false;
    }

    return true;
}

bool ReadWriteLibarchivePlugin::initializeNewFileCompressionOptions(const CompressionOptions &options)
{
    int ret;
    bool requiresExecutable = false;
    const auto threads = std::to_string(std::max(1u, static_cast<unsigned>(std::thread::hardware_concurrency() * 0.8)));
    const bool is7zFile = filename().endsWith(QLatin1String("7z"), Qt::CaseInsensitive);

    if (is7zFile) {
        // 7zip format doesn't need any filter to be set.
    } else if (filename().endsWith(QLatin1String("gz"), Qt::CaseInsensitive)) {
        qCDebug(ARK_LOG) << "Detected gzip compression for new file";
        ret = archive_write_add_filter_gzip(m_archiveWriter.data());
    } else if (filename().endsWith(QLatin1String("bz2"), Qt::CaseInsensitive)) {
        qCDebug(ARK_LOG) << "Detected bzip2 compression for new file";
        ret = archive_write_add_filter_bzip2(m_archiveWriter.data());
    } else if (filename().endsWith(QLatin1String("xz"), Qt::CaseInsensitive)) {
        qCDebug(ARK_LOG) << "Detected xz compression for new file";
        ret = archive_write_add_filter_xz(m_archiveWriter.data());

        // Set number of threads.
        ret = archive_write_set_filter_option(m_archiveWriter.data(), "xz", "threads", threads.c_str());
        if (ret != ARCHIVE_OK) {
            qCWarning(ARK_LOG) << "Failed to set number of threads, fallback to single thread mode" << archive_error_string(m_archiveWriter.data());
        }
    } else if (filename().endsWith(QLatin1String("lzma"), Qt::CaseInsensitive)) {
        qCDebug(ARK_LOG) << "Detected lzma compression for new file";
        ret = archive_write_add_filter_lzma(m_archiveWriter.data());
    } else if (filename().endsWith(QLatin1String(".z"), Qt::CaseInsensitive)) {
        qCDebug(ARK_LOG) << "Detected compress (.Z) compression for new file";
        ret = archive_write_add_filter_compress(m_archiveWriter.data());
    } else if (filename().endsWith(QLatin1String("lz"), Qt::CaseInsensitive)) {
        qCDebug(ARK_LOG) << "Detected lzip compression for new file";
        ret = archive_write_add_filter_lzip(m_archiveWriter.data());
    } else if (filename().endsWith(QLatin1String("lzo"), Qt::CaseInsensitive)) {
        qCDebug(ARK_LOG) << "Detected lzop compression for new file";
        ret = archive_write_add_filter_lzop(m_archiveWriter.data());
    } else if (filename().endsWith(QLatin1String("lrz"), Qt::CaseInsensitive)) {
        qCDebug(ARK_LOG) << "Detected lrzip compression for new file";
        ret = archive_write_add_filter_lrzip(m_archiveWriter.data());
        requiresExecutable = true;
    } else if (filename().endsWith(QLatin1String("lz4"), Qt::CaseInsensitive)) {
        qCDebug(ARK_LOG) << "Detected lz4 compression for new file";
        ret = archive_write_add_filter_lz4(m_archiveWriter.data());
    } else if (filename().endsWith(QLatin1String("zst"), Qt::CaseInsensitive)) {
        qCDebug(ARK_LOG) << "Detected zstd compression for new file";
        ret = archive_write_add_filter_zstd(m_archiveWriter.data());

        // Set number of threads.
        ret = archive_write_set_filter_option(m_archiveWriter.data(), "zstd", "threads", threads.c_str());
        if (ret != ARCHIVE_OK) {
            qCWarning(ARK_LOG) << "Failed to set number of threads, fallback to single thread mode" << archive_error_string(m_archiveWriter.data());
        }
    } else if (filename().endsWith(QLatin1String("tar"), Qt::CaseInsensitive)) {
        qCDebug(ARK_LOG) << "Detected no compression for new file (pure tar)";
        ret = archive_write_add_filter_none(m_archiveWriter.data());
    } else {
        qCDebug(ARK_LOG) << "Falling back to gzip";
        ret = archive_write_add_filter_gzip(m_archiveWriter.data());
    }

    // Libarchive emits a warning for lrzip due to using external executable.
    if ((requiresExecutable && ret != ARCHIVE_WARN) || (!requiresExecutable && ret != ARCHIVE_OK)) {
        qCWarning(ARK_LOG) << "Failed to set compression method:" << archive_error_string(m_archiveWriter.data());
        Q_EMIT error(i18nc("@info", "Could not set the compression method."));
        return false;
    }

    // If the format supports multiple compression methods (e.g. 7zip does), set the configured method.
    if (!options.compressionMethod().isEmpty()) {
        QString compressionMethod = options.compressionMethod().toLower();
        qCDebug(ARK_LOG) << "Using compression method:" << compressionMethod;
        ret = archive_write_set_format_option(m_archiveWriter.data(), nullptr, "compression", compressionMethod.toUtf8().constData());

        if (ret != ARCHIVE_OK) {
            qCWarning(ARK_LOG) << "Failed to set compression method" << archive_error_string(m_archiveWriter.data());
            Q_EMIT error(i18nc("@info", "Could not set the compression method."));
            return false;
        }
    }

    // Set compression level if passed in CompressionOptions.
    if (options.isCompressionLevelSet()) {
        qCDebug(ARK_LOG) << "Using compression level:" << options.compressionLevel();
        // 7zip supports the compression level as format option (it doesn't have any filter).
        if (is7zFile) {
            ret = archive_write_set_format_option(m_archiveWriter.data(),
                                                  nullptr,
                                                  "compression-level",
                                                  QString::number(options.compressionLevel()).toUtf8().constData());
        } else {
            ret = archive_write_set_filter_option(m_archiveWriter.data(),
                                                  nullptr,
                                                  "compression-level",
                                                  QString::number(options.compressionLevel()).toUtf8().constData());
        }

        if (ret != ARCHIVE_OK) {
            qCWarning(ARK_LOG) << "Failed to set compression level" << archive_error_string(m_archiveWriter.data());
            Q_EMIT error(i18nc("@info", "Could not set the compression level."));
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
                    Q_EMIT entryRemoved(file);
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
                Q_EMIT entryRemoved(file);
                Q_EMIT progress(float(newEntries + entriesCounter + iteratedEntries) / float(totalCount));
                break;

            case Add:
                qCDebug(ARK_LOG) << file << "is already present in the new archive, skipping.";
                // When overwriting entries, we need to decrement the counter manually,
                // because entry was emitted.
                m_numberOfEntries--;
                break;

            default:
                qCDebug(ARK_LOG) << "Mode" << mode << "is not considered for processing old libarchive entries";
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
        Q_EMIT progress(float(newEntries + entriesCounter + iteratedEntries) / float(totalCount));
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
        qCCritical(ARK_LOG) << "archive_write_header() has returned" << returnCode << "with errno" << archive_errno(m_archiveWriter.data());
        Q_EMIT error(i18nc("@info", "Could not compress entry, operation aborted."));
        return false;
    default:
        qCDebug(ARK_LOG) << "archive_writer_header() has returned" << returnCode << "which will be ignored.";
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

    struct stat st;
#ifndef Q_OS_WIN
    // #253059: Even if we use archive_read_disk_entry_from_file,
    //          libarchive may have been compiled without HAVE_LSTAT,
    //          or something may have caused it to follow symlinks, in
    //          which case stat() will be called. To avoid this, we
    //          call lstat() ourselves.

    lstat(QFile::encodeName(absoluteFilename).constData(), &st); // krazy:exclude=syscalls
#endif

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
        qCCritical(ARK_LOG) << "Writing header failed with error code " << returnCode;
        qCCritical(ARK_LOG) << "Error while writing..." << archive_error_string(m_archiveWriter.data())
                            << "(error no =" << archive_errno(m_archiveWriter.data()) << ')';

        Q_EMIT error(i18nc("@info Error in a message box", "Could not compress entry."));

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

#include "moc_readwritelibarchiveplugin.cpp"
#include "readwritelibarchiveplugin.moc"
