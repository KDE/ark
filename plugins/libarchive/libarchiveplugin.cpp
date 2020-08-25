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

#include "libarchiveplugin.h"
#include "ark_debug.h"
#include "queries.h"

#include <KLocalizedString>

#include <QThread>
#include <QFileInfo>
#include <QDir>

#include <archive_entry.h>

LibarchivePlugin::LibarchivePlugin(QObject *parent, const QVariantList &args)
    : ReadWriteArchiveInterface(parent, args)
    , m_archiveReadDisk(archive_read_disk_new())
    , m_cachedArchiveEntryCount(0)
    , m_emitNoEntries(false)
    , m_extractedFilesSize(0)
{
    qCDebug(ARK) << "Initializing libarchive plugin";
    archive_read_disk_set_standard_lookup(m_archiveReadDisk.data());

    connect(this, &ReadOnlyArchiveInterface::error, this, &LibarchivePlugin::slotRestoreWorkingDir);
    connect(this, &ReadOnlyArchiveInterface::cancelled, this, &LibarchivePlugin::slotRestoreWorkingDir);
}

LibarchivePlugin::~LibarchivePlugin()
{
    for (const auto e : qAsConst(m_emittedEntries)) {
        // Entries might be passed to pending slots, so we just schedule their deletion.
        e->deleteLater();
    }
}

bool LibarchivePlugin::list()
{
    qCDebug(ARK) << "Listing archive contents";

    if (!initializeReader()) {
        return false;
    }

    qCDebug(ARK) << "Detected compression filter:" << archive_filter_name(m_archiveReader.data(), 0);
    QString compMethod = convertCompressionName(QString::fromUtf8(archive_filter_name(m_archiveReader.data(), 0)));
    if (!compMethod.isEmpty()) {
        emit compressionMethodFound(compMethod);
    }

    m_cachedArchiveEntryCount = 0;
    m_extractedFilesSize = 0;
    m_numberOfEntries = 0;
    auto compressedArchiveSize = QFileInfo(filename()).size();

    struct archive_entry *aentry;
    int result = ARCHIVE_RETRY;

    bool firstEntry = true;
    while (!QThread::currentThread()->isInterruptionRequested() && (result = archive_read_next_header(m_archiveReader.data(), &aentry)) == ARCHIVE_OK) {

        if (firstEntry) {
            qCDebug(ARK) << "Detected format for first entry:" << archive_format_name(m_archiveReader.data());
            firstEntry = false;
        }

        if (!m_emitNoEntries) {
            emitEntryFromArchiveEntry(aentry);
        }

        m_extractedFilesSize += (qlonglong)archive_entry_size(aentry);

        emit progress(float(archive_filter_bytes(m_archiveReader.data(), -1))/float(compressedArchiveSize));

        m_cachedArchiveEntryCount++;

        // Skip the entry data.
        int readSkipResult = archive_read_data_skip(m_archiveReader.data());
        if (readSkipResult != ARCHIVE_OK) {
            qCCritical(ARK) << "Error while skipping data for entry:"
                            << QString::fromWCharArray(archive_entry_pathname_w(aentry))
                            << readSkipResult
                            << QLatin1String(archive_error_string(m_archiveReader.data()));
            if (!emitCorruptArchive()) {
                return false;
            }
        }
    }

    if (result != ARCHIVE_EOF) {
        qCCritical(ARK) << "Error while reading archive:"
                        << result
                        << QLatin1String(archive_error_string(m_archiveReader.data()));
        if (!emitCorruptArchive()) {
            return false;
        }
    }

    return archive_read_close(m_archiveReader.data()) == ARCHIVE_OK;
}

bool LibarchivePlugin::emitCorruptArchive()
{
    Kerfuffle::LoadCorruptQuery query(filename());
    emit userQuery(&query);
    query.waitForResponse();
    if (!query.responseYes()) {
        emit cancelled();
        archive_read_close(m_archiveReader.data());
        return false;
    } else {
        emit progress(1.0);
        return true;
    }
}

bool LibarchivePlugin::addFiles(const QVector<Archive::Entry*> &files, const Archive::Entry *destination, const CompressionOptions &options, uint numberOfEntriesToAdd)
{
    Q_UNUSED(files)
    Q_UNUSED(destination)
    Q_UNUSED(options)
    Q_UNUSED(numberOfEntriesToAdd)
    return false;
}

bool LibarchivePlugin::moveFiles(const QVector<Archive::Entry*> &files, Archive::Entry *destination, const CompressionOptions &options)
{
    Q_UNUSED(files)
    Q_UNUSED(destination)
    Q_UNUSED(options)
    return false;
}

bool LibarchivePlugin::copyFiles(const QVector<Archive::Entry*> &files, Archive::Entry *destination, const CompressionOptions &options)
{
    Q_UNUSED(files)
    Q_UNUSED(destination)
    Q_UNUSED(options)
    return false;
}

bool LibarchivePlugin::deleteFiles(const QVector<Archive::Entry*> &files)
{
    Q_UNUSED(files)
    return false;
}

bool LibarchivePlugin::addComment(const QString &comment)
{
    Q_UNUSED(comment)
    return false;
}

bool LibarchivePlugin::testArchive()
{
    return false;
}

bool LibarchivePlugin::hasBatchExtractionProgress() const
{
    return true;
}

bool LibarchivePlugin::doKill()
{
    return false;
}

bool LibarchivePlugin::extractFiles(const QVector<Archive::Entry*> &files, const QString &destinationDirectory, const ExtractionOptions &options)
{
    if (!initializeReader()) {
        return false;
    }

    ArchiveWrite writer(archive_write_disk_new());
    if (!writer.data()) {
        return false;
    }

    archive_write_disk_set_options(writer.data(), extractionFlags());

    int totalEntriesCount = 0;
    const bool extractAll = files.isEmpty();
    if (extractAll) {
        if (!m_cachedArchiveEntryCount) {
            emit progress(0);
            //TODO: once information progress has been implemented, send
            //feedback here that the archive is being read
            qCDebug(ARK) << "For getting progress information, the archive will be listed once";
            m_emitNoEntries = true;
            list();
            m_emitNoEntries = false;
        }
        totalEntriesCount = m_cachedArchiveEntryCount;
    } else {
        totalEntriesCount = files.size();
    }

    qCDebug(ARK) << "Going to extract" << totalEntriesCount << "entries";

    qCDebug(ARK) << "Changing current directory to " << destinationDirectory;
    m_oldWorkingDir = QDir::currentPath();
    QDir::setCurrent(destinationDirectory);

    // Initialize variables.
    const bool preservePaths = options.preservePaths();
    const bool removeRootNode = options.isDragAndDropEnabled();
    bool overwriteAll = false; // Whether to overwrite all files
    bool skipAll = false; // Whether to skip all files
    bool dontPromptErrors = false; // Whether to prompt for errors
    m_currentExtractedFilesSize = 0;
    int extractedEntriesCount = 0;
    int progressEntryCount = 0;
    struct archive_entry *entry;
    QString fileBeingRenamed;
    // To avoid traversing the entire archive when extracting a limited set of
    // entries, we maintain a list of remaining entries and stop when it's empty.
    const QStringList fullPaths = entryFullPaths(files);
    QStringList remainingFiles = entryFullPaths(files);

    // Iterate through all entries in archive.
    while (!QThread::currentThread()->isInterruptionRequested() && (archive_read_next_header(m_archiveReader.data(), &entry) == ARCHIVE_OK)) {

        if (!extractAll && remainingFiles.isEmpty()) {
            break;
        }

        fileBeingRenamed.clear();
        int index = -1;

        // Retry with renamed entry, fire an overwrite query again
        // if the new entry also exists.
    retry:
        const bool entryIsDir = S_ISDIR(archive_entry_mode(entry));
        // Skip directories if not preserving paths.
        if (!preservePaths && entryIsDir) {
            archive_read_data_skip(m_archiveReader.data());
            continue;
        }

        // entryName is the name inside the archive, full path
        QString entryName = QDir::fromNativeSeparators(QFile::decodeName(archive_entry_pathname(entry)));

        // Some archive types e.g. AppImage prepend all entries with "./" so remove this part.
        if (entryName.startsWith(QLatin1String("./"))) {
            entryName.remove(0, 2);
        }

        // Static libraries (*.a) contain the two entries "/" and "//".
        // We just skip these to allow extracting this archive type.
        if (entryName == QLatin1String("/") || entryName == QLatin1String("//")) {
            archive_read_data_skip(m_archiveReader.data());
            continue;
        }

        // For now we just can't handle absolute filenames in a tar archive.
        // TODO: find out what to do here!!
        if (entryName.startsWith(QLatin1Char( '/' ))) {
            emit error(i18n("This archive contains archive entries with absolute paths, "
                            "which are not supported by Ark."));
            return false;
        }

        // Should the entry be extracted?
        if (extractAll ||
            remainingFiles.contains(entryName) ||
            entryName == fileBeingRenamed) {

            // Find the index of entry.
            if (entryName != fileBeingRenamed) {
                index = fullPaths.indexOf(entryName);
            }
            if (!extractAll && index == -1) {
                // If entry is not found in files, skip entry.
                continue;
            }

            // entryFI is the fileinfo pointing to where the file will be
            // written from the archive.
            QFileInfo entryFI(entryName);
            //qCDebug(ARK) << "setting path to " << archive_entry_pathname( entry );

            const QString fileWithoutPath(entryFI.fileName());
            // If we DON'T preserve paths, we cut the path and set the entryFI
            // fileinfo to the one without the path.
            if (!preservePaths) {
                // Empty filenames (ie dirs) should have been skipped already,
                // so asserting.
                Q_ASSERT(!fileWithoutPath.isEmpty());
                archive_entry_copy_pathname(entry, QFile::encodeName(fileWithoutPath).constData());
                entryFI = QFileInfo(fileWithoutPath);

            // OR, if the file has a rootNode attached, remove it from file path.
            } else if (!extractAll && removeRootNode && entryName != fileBeingRenamed) {
                const QString &rootNode = files.at(index)->rootNode;
                if (!rootNode.isEmpty()) {
                    const QString truncatedFilename(entryName.remove(entryName.indexOf(rootNode), rootNode.size()));
                    archive_entry_copy_pathname(entry, QFile::encodeName(truncatedFilename).constData());
                    entryFI = QFileInfo(truncatedFilename);
                }
            }

            // Check if the file about to be written already exists.
            if (!entryIsDir && entryFI.exists()) {
                if (skipAll) {
                    archive_read_data_skip(m_archiveReader.data());
                    archive_entry_clear(entry);
                    continue;
                } else if (!overwriteAll && !skipAll) {
                    Kerfuffle::OverwriteQuery query(entryName);
                    emit userQuery(&query);
                    query.waitForResponse();

                    if (query.responseCancelled()) {
                        emit cancelled();
                        archive_read_data_skip(m_archiveReader.data());
                        archive_entry_clear(entry);
                        break;
                    } else if (query.responseSkip()) {
                        archive_read_data_skip(m_archiveReader.data());
                        archive_entry_clear(entry);
                        continue;
                    } else if (query.responseAutoSkip()) {
                        archive_read_data_skip(m_archiveReader.data());
                        archive_entry_clear(entry);
                        skipAll = true;
                        continue;
                    } else if (query.responseRename()) {
                        const QString newName(query.newFilename());
                        fileBeingRenamed = newName;
                        archive_entry_copy_pathname(entry, QFile::encodeName(newName).constData());
                        goto retry;
                    } else if (query.responseOverwriteAll()) {
                        overwriteAll = true;
                    }
                }
            }

            // If there is an already existing directory.
            if (entryIsDir && entryFI.exists()) {
                if (entryFI.isWritable()) {
                    qCWarning(ARK) << "Warning, existing, but writable dir";
                } else {
                    qCWarning(ARK) << "Warning, existing, but non-writable dir. skipping";
                    archive_entry_clear(entry);
                    archive_read_data_skip(m_archiveReader.data());
                    continue;
                }
            }

            // Write the entry header and check return value.
            const int returnCode = archive_write_header(writer.data(), entry);
            switch (returnCode) {
            case ARCHIVE_OK:
                // If the whole archive is extracted and the total filesize is
                // available, we use partial progress.
                copyData(entryName, m_archiveReader.data(), writer.data(), (extractAll && m_extractedFilesSize));
                break;

            case ARCHIVE_FAILED:
                qCCritical(ARK) << "archive_write_header() has returned" << returnCode
                                << "with errno" << archive_errno(writer.data());

                // If they user previously decided to ignore future errors,
                // don't bother prompting again.
                if (!dontPromptErrors) {
                    // Ask the user if he wants to continue extraction despite an error for this entry.
                    Kerfuffle::ContinueExtractionQuery query(QLatin1String(archive_error_string(writer.data())),
                                                             entryName);
                    emit userQuery(&query);
                    query.waitForResponse();

                    if (query.responseCancelled()) {
                        emit cancelled();
                        return false;
                    }
                    dontPromptErrors = query.dontAskAgain();
                }
                break;

            case ARCHIVE_FATAL:
                qCCritical(ARK) << "archive_write_header() has returned" << returnCode
                                << "with errno" << archive_errno(writer.data());
                emit error(i18nc("@info", "Fatal error, extraction aborted."));
                return false;
            default:
                qCDebug(ARK) << "archive_write_header() returned" << returnCode
                             << "which will be ignored.";
                break;
            }

            // If we only partially extract the archive and the number of
            // archive entries is available we use a simple progress based on
            // number of items extracted.
            if (!extractAll && m_cachedArchiveEntryCount) {
                ++progressEntryCount;
                emit progress(float(progressEntryCount) / totalEntriesCount);
            }

            extractedEntriesCount++;
            remainingFiles.removeOne(entryName);
        } else {
            // Archive entry not among selected files, skip it.
            archive_read_data_skip(m_archiveReader.data());
        }
    }

    qCDebug(ARK) << "Extracted" << extractedEntriesCount << "entries";
    slotRestoreWorkingDir();
    return archive_read_close(m_archiveReader.data()) == ARCHIVE_OK;
}

bool LibarchivePlugin::initializeReader()
{
    m_archiveReader.reset(archive_read_new());

    if (!(m_archiveReader.data())) {
        emit error(i18n("The archive reader could not be initialized."));
        return false;
    }

    if (archive_read_support_filter_all(m_archiveReader.data()) != ARCHIVE_OK) {
        return false;
    }

    if (archive_read_support_format_all(m_archiveReader.data()) != ARCHIVE_OK) {
        return false;
    }

    if (archive_read_open_filename(m_archiveReader.data(), QFile::encodeName(filename()).constData(), 10240) != ARCHIVE_OK) {
        qCWarning(ARK) << "Could not open the archive:" << archive_error_string(m_archiveReader.data());
        emit error(i18nc("@info", "Archive corrupted or insufficient permissions."));
        return false;
    }

    return true;
}

void LibarchivePlugin::emitEntryFromArchiveEntry(struct archive_entry *aentry)
{
    auto e = new Archive::Entry();

#ifdef Q_OS_WIN
    e->setProperty("fullPath", QDir::fromNativeSeparators(QString::fromUtf16((ushort*)archive_entry_pathname_w(aentry))));
#else
    e->setProperty("fullPath", QDir::fromNativeSeparators(QString::fromWCharArray(archive_entry_pathname_w(aentry))));
#endif

    const QString owner = QString::fromLatin1(archive_entry_uname(aentry));
    if (!owner.isEmpty()) {
        e->setProperty("owner", owner);
    } else {
        e->setProperty("owner", static_cast<qlonglong>(archive_entry_uid(aentry)));
    }

    const QString group = QString::fromLatin1(archive_entry_gname(aentry));
    if (!group.isEmpty()) {
        e->setProperty("group", group);
    } else {
        e->setProperty("group", static_cast<qlonglong>(archive_entry_gid(aentry)));
    }

    const mode_t mode = archive_entry_mode(aentry);
    if (mode != 0) {
        e->setProperty("permissions", QString::number(mode, 8));
    }
    e->setProperty("isExecutable", mode & (S_IXUSR | S_IXGRP | S_IXOTH));

    e->compressedSizeIsSet = false;
    e->setProperty("size", (qlonglong)archive_entry_size(aentry));
    e->setProperty("isDirectory", S_ISDIR(archive_entry_mode(aentry)));

    if (archive_entry_symlink(aentry)) {
        e->setProperty("link", QLatin1String( archive_entry_symlink(aentry) ));
    }

    auto time = static_cast<uint>(archive_entry_mtime(aentry));
    e->setProperty("timestamp", QDateTime::fromSecsSinceEpoch(time));

    emit entry(e);
    m_emittedEntries << e;
}

int LibarchivePlugin::extractionFlags() const
{
    return ARCHIVE_EXTRACT_TIME
           | ARCHIVE_EXTRACT_SECURE_NODOTDOT
           | ARCHIVE_EXTRACT_SECURE_SYMLINKS;
}

void LibarchivePlugin::copyData(const QString& filename, struct archive *dest, bool partialprogress)
{
    char buff[10240];
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    auto readBytes = file.read(buff, sizeof(buff));
    while (readBytes > 0 && !QThread::currentThread()->isInterruptionRequested()) {
        archive_write_data(dest, buff, static_cast<size_t>(readBytes));
        if (archive_errno(dest) != ARCHIVE_OK) {
            qCCritical(ARK) << "Error while writing" << filename << ":" << archive_error_string(dest)
                            << "(error no =" << archive_errno(dest) << ')';
            return;
        }

        if (partialprogress) {
            m_currentExtractedFilesSize += readBytes;
            emit progress(float(m_currentExtractedFilesSize) / m_extractedFilesSize);
        }

        readBytes = file.read(buff, sizeof(buff));
    }

    file.close();
}

void LibarchivePlugin::copyData(const QString& filename, struct archive *source, struct archive *dest, bool partialprogress)
{
    char buff[10240];

    auto readBytes = archive_read_data(source, buff, sizeof(buff));
    while (readBytes > 0 && !QThread::currentThread()->isInterruptionRequested()) {
        archive_write_data(dest, buff, static_cast<size_t>(readBytes));
        if (archive_errno(dest) != ARCHIVE_OK) {
            qCCritical(ARK) << "Error while extracting" << filename << ":" << archive_error_string(dest)
                            << "(error no =" << archive_errno(dest) << ')';
            return;
        }

        if (partialprogress) {
            m_currentExtractedFilesSize += readBytes;
            emit progress(float(m_currentExtractedFilesSize) / m_extractedFilesSize);
        }

        readBytes = archive_read_data(source, buff, sizeof(buff));
    }
}

void LibarchivePlugin::slotRestoreWorkingDir()
{
    if (m_oldWorkingDir.isEmpty()) {
        return;
    }

    if (!QDir::setCurrent(m_oldWorkingDir)) {
        qCWarning(ARK) << "Failed to restore old working directory:" << m_oldWorkingDir;
    } else {
        m_oldWorkingDir.clear();
    }
}

QString LibarchivePlugin::convertCompressionName(const QString &method)
{
    if (method == QLatin1String("gzip")) {
        return QStringLiteral("GZip");
    } else if (method == QLatin1String("bzip2")) {
        return QStringLiteral("BZip2");
    } else if (method == QLatin1String("xz")) {
        return QStringLiteral("XZ");
    } else if (method == QLatin1String("compress (.Z)")) {
        return QStringLiteral("Compress");
    } else if (method == QLatin1String("lrzip")) {
        return QStringLiteral("LRZip");
    } else if (method == QLatin1String("lzip")) {
        return QStringLiteral("LZip");
    } else if (method == QLatin1String("lz4")) {
        return QStringLiteral("LZ4");
    } else if (method == QLatin1String("lzop")) {
        return QStringLiteral("lzop");
    } else if (method == QLatin1String("lzma")) {
        return QStringLiteral("LZMA");
    } else if (method == QLatin1String("zstd")) {
        return QStringLiteral("Zstandard");
    }
    return QString();
}

