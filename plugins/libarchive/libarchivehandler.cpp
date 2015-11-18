/*
 * Copyright (c) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (c) 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (c) 2010 Raphael Kubo da Costa <rakuco@FreeBSD.org>
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

#include "libarchivehandler.h"
#include "ark_debug.h"
#include "kerfuffle/kerfuffle_export.h"
#include "kerfuffle/queries.h"

#include <archive.h>
#include <archive_entry.h>

#include <KLocalizedString>
#include <KPluginFactory>

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QList>
#include <QStringList>
#include <QSaveFile>

K_PLUGIN_FACTORY( LibArchivePluginFactory, registerPlugin< LibArchiveInterface >(); )

struct LibArchiveInterface::ArchiveReadCustomDeleter
{
    static inline void cleanup(struct archive *a)
    {
        if (a) {
            archive_read_free(a);
        }
    }
};

struct LibArchiveInterface::ArchiveWriteCustomDeleter
{
    static inline void cleanup(struct archive *a)
    {
        if (a) {
            archive_write_free(a);
        }
    }
};

LibArchiveInterface::LibArchiveInterface(QObject *parent, const QVariantList & args)
    : ReadWriteArchiveInterface(parent, args)
    , m_cachedArchiveEntryCount(0)
    , m_emitNoEntries(false)
    , m_extractedFilesSize(0)
    , m_workDir(QDir::current())
    , m_archiveReadDisk(archive_read_disk_new())
    , m_abortOperation(false)
{
    qCDebug(ARK) << "Loaded libarchive plugin";
    archive_read_disk_set_standard_lookup(m_archiveReadDisk.data());
}

LibArchiveInterface::~LibArchiveInterface()
{
}

bool LibArchiveInterface::list()
{
    qCDebug(ARK) << "Listing archive contents";

    ArchiveRead arch_reader(archive_read_new());

    if (!(arch_reader.data())) {
        return false;
    }

    if (archive_read_support_filter_all(arch_reader.data()) != ARCHIVE_OK) {
        return false;
    }

    if (archive_read_support_format_all(arch_reader.data()) != ARCHIVE_OK) {
        return false;
    }

    if (archive_read_open_filename(arch_reader.data(), QFile::encodeName(filename()), 10240) != ARCHIVE_OK) {
        emit error(xi18nc("@info", "Could not open the archive <filename>%1</filename>, libarchive cannot handle it.",
                   filename()));
        return false;
    }

    m_cachedArchiveEntryCount = 0;
    m_extractedFilesSize = 0;

    struct archive_entry *aentry;
    int result;

    while (!m_abortOperation && (result = archive_read_next_header(arch_reader.data(), &aentry)) == ARCHIVE_OK) {
        if (!m_emitNoEntries) {
            emitEntryFromArchiveEntry(aentry);
        }

        m_extractedFilesSize += (qlonglong)archive_entry_size(aentry);

        m_cachedArchiveEntryCount++;
        archive_read_data_skip(arch_reader.data());
    }
    m_abortOperation = false;

    if (result != ARCHIVE_EOF) {
        emit error(xi18nc("@info", "The archive reading failed with the following error: <message>%1</message>",
                   QLatin1String( archive_error_string(arch_reader.data()))));
        return false;
    }

    return archive_read_close(arch_reader.data()) == ARCHIVE_OK;
}

bool LibArchiveInterface::doKill()
{
    m_abortOperation = true;
    return true;
}

bool LibArchiveInterface::copyFiles(const QVariantList& files, const QString& destinationDirectory, const ExtractionOptions& options)
{
    qCDebug(ARK) << "Changing current directory to " << destinationDirectory;
    QDir::setCurrent(destinationDirectory);

    const bool extractAll = files.isEmpty();
    const bool preservePaths = options.value(QStringLiteral( "PreservePaths" )).toBool();
    bool removeRootNode = options.value(QStringLiteral("RemoveRootNode"), QVariant()).toBool();

    // See if there is a singular RootNode.
    QString rootNodeSingular = options.value(QStringLiteral("RootNode"), QVariant()).toString();
    if (!rootNodeSingular.isEmpty() && !rootNodeSingular.endsWith(QLatin1Char('/'))) {
        rootNodeSingular.append(QLatin1Char('/'));
    }

    // To avoid traversing the entire archive when extracting a limited set of
    // entries, we maintain a list of remaining entries and stop when it's
    // empty.
    QVariantList remainingFiles = files;

    ArchiveRead arch(archive_read_new());

    if (!(arch.data())) {
        return false;
    }

    if (archive_read_support_filter_all(arch.data()) != ARCHIVE_OK) {
        return false;
    }

    if (archive_read_support_format_all(arch.data()) != ARCHIVE_OK) {
        return false;
    }

    if (archive_read_open_filename(arch.data(), QFile::encodeName(filename()), 10240) != ARCHIVE_OK) {
        emit error(xi18nc("@info", "Could not open the archive <filename>%1</filename>, libarchive cannot handle it.",
                          filename()));
        return false;
    }

    ArchiveWrite writer(archive_write_disk_new());
    if (!writer.data()) {
        return false;
    }

    archive_write_disk_set_options(writer.data(), extractionFlags());

    int entryNr = 0;
    int totalCount = 0;

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
        totalCount = m_cachedArchiveEntryCount;
    } else {
        totalCount = files.size();
    }

    qCDebug(ARK) << "Going to extract" << totalCount << "entries";


    // Initialize variables.
    bool overwriteAll = false; // Whether to overwrite all files
    bool skipAll = false; // Whether to skip all files
    bool dontPromptErrors = false; // Whether to prompt for errors
    m_currentExtractedFilesSize = 0;
    int no_entries = 0;

    struct archive_entry *entry;
    QString fileBeingRenamed;

    // Iterate through all entries in archive.
    while (archive_read_next_header(arch.data(), &entry) == ARCHIVE_OK) {

        if (!extractAll && remainingFiles.isEmpty()) {
            break;
        }

        fileBeingRenamed.clear();
        int index;

        // Retry with renamed entry, fire an overwrite query again
        // if the new entry also exists.
    retry:

        const bool entryIsDir = S_ISDIR(archive_entry_mode(entry));

        // Skip directories if not preserving paths.
        if (!preservePaths && entryIsDir) {
            archive_read_data_skip(arch.data());
            continue;
        }

        // entryName is the name inside the archive, full path
        QString entryName = QDir::fromNativeSeparators(QFile::decodeName(archive_entry_pathname(entry)));

        // For now we just can't handle absolute filenames in a tar archive.
        // TODO: find out what to do here!!
        if (entryName.startsWith(QLatin1Char( '/' ))) {
            emit error(i18n("This archive contains archive entries with absolute paths, "
                            "which are not supported by Ark."));
            return false;
        }

        // Should the entry be extracted?
        if (extractAll ||
            remainingFiles.contains(QVariant::fromValue(fileRootNodePair(entryName))) ||
            entryName == fileBeingRenamed) {

            // Find the index of entry.
            if (entryName != fileBeingRenamed) {
                index = files.indexOf(QVariant::fromValue(fileRootNodePair(entryName)));
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
            } else if (!extractAll && removeRootNode && entryName != fileBeingRenamed &&
                       !files.at(index).value<fileRootNodePair>().rootNode.isEmpty()) {

                //qCDebug(ARK) << "Removing" << files.at(index).value<fileRootNodePair>().rootNode << "from" << entryName;

                const QString truncatedFilename(entryName.remove(0, files.at(index).value<fileRootNodePair>().rootNode.size()));
                archive_entry_copy_pathname(entry, QFile::encodeName(truncatedFilename).constData());
                entryFI = QFileInfo(truncatedFilename);

            // OR, if a singular rootNode is provided, remove it from file path.
            } else if (removeRootNode &&
                       entryName != fileBeingRenamed &&
                       !rootNodeSingular.isEmpty()) {

                //qCDebug(ARK) << "Removing" << rootNodeSingular << "from" << entryName;

                const QString truncatedFilename(entryName.remove(0, rootNodeSingular.size()));
                archive_entry_copy_pathname(entry, QFile::encodeName(truncatedFilename).constData());
                entryFI = QFileInfo(truncatedFilename);
            }

            // Check if the file about to be written already exists.
            if (!entryIsDir && entryFI.exists()) {
                if (skipAll) {
                    archive_read_data_skip(arch.data());
                    archive_entry_clear(entry);
                    continue;
                } else if (!overwriteAll && !skipAll) {
                    Kerfuffle::OverwriteQuery query(entryName);
                    emit userQuery(&query);
                    query.waitForResponse();

                    if (query.responseCancelled()) {
                        archive_read_data_skip(arch.data());
                        archive_entry_clear(entry);
                        break;
                    } else if (query.responseSkip()) {
                        archive_read_data_skip(arch.data());
                        archive_entry_clear(entry);
                        continue;
                    } else if (query.responseAutoSkip()) {
                        archive_read_data_skip(arch.data());
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
                    archive_read_data_skip(arch.data());
                    continue;
                }
            }

            // Write the entry header and check return value.
            const int returnCode = archive_write_header(writer.data(), entry);
            switch (returnCode) {
            case ARCHIVE_OK:
                // If the whole archive is extracted and the total filesize is
                // available, we use partial progress.
                copyData(entryName, arch.data(), writer.data(), (extractAll && m_extractedFilesSize));
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
                emit error(xi18nc("@info", "Extraction failed at:<nl/><filename>%1</filename>",
                                  entryName));
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
                ++entryNr;
                emit progress(float(entryNr) / totalCount);
            }
            archive_entry_clear(entry);
            no_entries++;

            remainingFiles.removeOne(QVariant::fromValue(fileRootNodePair(entryName)));

        } else {

            // Archive entry not among selected files, skip it.
            archive_read_data_skip(arch.data());

        }

    } // While entries left to read in archive.

    qCDebug(ARK) << "Extracted" << no_entries << "entries";

    return archive_read_close(arch.data()) == ARCHIVE_OK;
}

bool LibArchiveInterface::addFiles(const QStringList& files, const CompressionOptions& options)
{
    qCDebug(ARK) << "Adding files" << files << "with CompressionOptions" << options;

    const bool creatingNewFile = !QFileInfo(filename()).exists();
    const QString globalWorkDir = options.value(QStringLiteral( "GlobalWorkDir" )).toString();

    if (!globalWorkDir.isEmpty()) {
        qCDebug(ARK) << "GlobalWorkDir is set, changing dir to " << globalWorkDir;
        m_workDir.setPath(globalWorkDir);
        QDir::setCurrent(globalWorkDir);
    }

    m_writtenFiles.clear();

    ArchiveRead arch_reader;
    if (!creatingNewFile) {
        arch_reader.reset(archive_read_new());
        if (!arch_reader.data()) {
            emit error(i18n("The archive reader could not be initialized."));
            return false;
        }

        if (archive_read_support_filter_all(arch_reader.data()) != ARCHIVE_OK) {
            return false;
        }

        if (archive_read_support_format_all(arch_reader.data()) != ARCHIVE_OK) {
            return false;
        }

        if (archive_read_open_filename(arch_reader.data(), QFile::encodeName(filename()), 10240) != ARCHIVE_OK) {
            emit error(i18n("The source file could not be read."));
            return false;
        }
    }

    // |tempFile| needs to be created before |arch_writer| so that when we go
    // out of scope in a `return false' case ArchiveWriteCustomDeleter is
    // called before destructor of QSaveFile (ie. we call archive_write_close()
    // before close()'ing the file descriptor).
    QSaveFile tempFile(filename());
    if (!tempFile.open(QIODevice::WriteOnly | QIODevice::Unbuffered)) {
        emit error(xi18nc("@info", "Failed to create a temporary file to compress <filename>%1</filename>.", filename()));
        return false;
    }

    ArchiveWrite arch_writer(archive_write_new());
    if (!(arch_writer.data())) {
        emit error(i18n("The archive writer could not be initialized."));
        return false;
    }

    // pax_restricted is the libarchive default, let's go with that.
    archive_write_set_format_pax_restricted(arch_writer.data());

    int ret;
    if (creatingNewFile) {
        if (filename().right(2).toUpper() == QLatin1String("GZ")) {
            qCDebug(ARK) << "Detected gzip compression for new file";
            ret = archive_write_add_filter_gzip(arch_writer.data());
        } else if (filename().right(3).toUpper() == QLatin1String("BZ2")) {
            qCDebug(ARK) << "Detected bzip2 compression for new file";
            ret = archive_write_add_filter_bzip2(arch_writer.data());
        } else if (filename().right(2).toUpper() == QLatin1String("XZ")) {
            qCDebug(ARK) << "Detected xz compression for new file";
            ret = archive_write_add_filter_xz(arch_writer.data());
        } else if (filename().right(4).toUpper() == QLatin1String("LZMA")) {
            qCDebug(ARK) << "Detected lzma compression for new file";
            ret = archive_write_add_filter_lzma(arch_writer.data());
        } else if (filename().right(3).toUpper() == QLatin1String("TAR")) {
            qCDebug(ARK) << "Detected no compression for new file (pure tar)";
            ret = archive_write_add_filter_none(arch_writer.data());
        } else {
            qCDebug(ARK) << "Falling back to gzip";
            ret = archive_write_add_filter_gzip(arch_writer.data());
        }

        if (ret != ARCHIVE_OK) {
            emit error(xi18nc("@info", "Setting the compression method failed with the following error:<nl/><message>%1</message>",
                              QLatin1String(archive_error_string(arch_writer.data()))));
            return false;
        }
    } else {
        switch (archive_filter_code(arch_reader.data(), 0)) {
        case ARCHIVE_FILTER_GZIP:
            ret = archive_write_add_filter_gzip(arch_writer.data());
            break;
        case ARCHIVE_FILTER_BZIP2:
            ret = archive_write_add_filter_bzip2(arch_writer.data());
            break;
        case ARCHIVE_FILTER_XZ:
            ret = archive_write_add_filter_xz(arch_writer.data());
            break;
        case ARCHIVE_FILTER_LZMA:
            ret = archive_write_add_filter_lzma(arch_writer.data());
            break;
        case ARCHIVE_FILTER_NONE:
            ret = archive_write_add_filter_none(arch_writer.data());
            break;
        default:
            emit error(i18n("The compression type '%1' is not supported by Ark.",
                            QLatin1String(archive_filter_name(arch_reader.data(), 0))));
            return false;
        }

        if (ret != ARCHIVE_OK) {
            emit error(xi18nc("@info", "Setting the compression method failed with the following error:<nl/><message>%1</message>",
                              QLatin1String(archive_error_string(arch_writer.data()))));
            return false;
        }
    }

    ret = archive_write_open_fd(arch_writer.data(), tempFile.handle());
    if (ret != ARCHIVE_OK) {
        emit error(xi18nc("@info", "Opening the archive for writing failed with the following error:<nl/><message>%1</message>",
                          QLatin1String(archive_error_string(arch_writer.data()))));
        return false;
    }

    // First write the new files.
    foreach(const QString& selectedFile, files) {
        if (!writeFile(selectedFile, arch_writer.data())) {
            return false;
        }

        // For directories, write all subfiles/folders.
        if (QFileInfo(selectedFile).isDir()) {
            QDirIterator it(selectedFile,
                            QDir::AllEntries | QDir::Readable |
                            QDir::Hidden | QDir::NoDotAndDotDot,
                            QDirIterator::Subdirectories);

            while (it.hasNext()) {
                QString path = it.next();

                if ((it.fileName() == QLatin1String("..")) ||
                    (it.fileName() == QLatin1String("."))) {
                    continue;
                }

                const bool isRealDir = it.fileInfo().isDir() && !it.fileInfo().isSymLink();

                if (isRealDir) {
                    path.append(QLatin1Char('/'));
                }

                if (!writeFile(path, arch_writer.data())) {
                    return false;
                }
            }
        }
    }

    struct archive_entry *entry;

    // If we have old archive entries.
    if (!creatingNewFile) {

        // Copy old entries from previous archive to new archive.
        while (archive_read_next_header(arch_reader.data(), &entry) == ARCHIVE_OK) {

            const QString entryName = QFile::decodeName(archive_entry_pathname(entry));

            if (m_writtenFiles.contains(entryName)) {
                archive_read_data_skip(arch_reader.data());
                qCDebug(ARK) << entryName << "is already present in the new archive, skipping.";
                continue;
            }

            const int returnCode = archive_write_header(arch_writer.data(), entry);

            switch (returnCode) {
            case ARCHIVE_OK:
                // If the whole archive is extracted and the total filesize is
                // available, we use partial progress.
                copyData(QLatin1String(archive_entry_pathname(entry)), arch_reader.data(), arch_writer.data(), false);
                break;
            case ARCHIVE_FAILED:
            case ARCHIVE_FATAL:
                qCCritical(ARK) << "archive_write_header() has returned" << returnCode
                                << "with errno" << archive_errno(arch_writer.data());
                emit error(xi18nc("@info", "Compression failed while processing:<nl/>"
                                  "<filename>%1</filename><nl/><nl/>Operation aborted.", entryName));
                QFile::remove(tempFile.fileName());
                return false;
            default:
                qCWarning(ARK) << "archive_writer_header() has returned" << returnCode
                               << "which will be ignored.";
                break;
            }
            archive_entry_clear(entry);
        }
    }

    // In the success case, we need to manually close the archive_writer before
    // calling QSaveFile::commit(), otherwise the latter will close() the
    // file descriptor archive_writer is still working on.
    // TODO: We need to abstract this code better so that we only deal with one
    // object that manages both QSaveFile and ArchiveWriter.
    archive_write_close(arch_writer.data());
    tempFile.commit();

    return true;
}

bool LibArchiveInterface::deleteFiles(const QVariantList& files)
{
    qCDebug(ARK) << "Deleting files" << files;

    ArchiveRead arch_reader(archive_read_new());
    if (!(arch_reader.data())) {
        emit error(i18n("The archive reader could not be initialized."));
        return false;
    }

    if (archive_read_support_filter_all(arch_reader.data()) != ARCHIVE_OK) {
        return false;
    }

    if (archive_read_support_format_all(arch_reader.data()) != ARCHIVE_OK) {
        return false;
    }

    if (archive_read_open_filename(arch_reader.data(), QFile::encodeName(filename()), 10240) != ARCHIVE_OK) {
        emit error(i18n("The source file could not be read."));
        return false;
    }

    // |tempFile| needs to be created before |arch_writer| so that when we go
    // out of scope in a `return false' case ArchiveWriteCustomDeleter is
    // called before destructor of QSaveFile (ie. we call archive_write_close()
    // before close()'ing the file descriptor).
    QSaveFile tempFile(filename());
    if (!tempFile.open(QIODevice::WriteOnly | QIODevice::Unbuffered)) {
        emit error(i18nc("@info", "Failed to create a temporary file."));
        return false;
    }

    ArchiveWrite arch_writer(archive_write_new());
    if (!(arch_writer.data())) {
        emit error(i18n("The archive writer could not be initialized."));
        return false;
    }

    // pax_restricted is the libarchive default, let's go with that.
    archive_write_set_format_pax_restricted(arch_writer.data());

    int ret;
    switch (archive_filter_code(arch_reader.data(), 0)) {
    case ARCHIVE_FILTER_GZIP:
        ret = archive_write_add_filter_gzip(arch_writer.data());
        break;
    case ARCHIVE_FILTER_BZIP2:
        ret = archive_write_add_filter_bzip2(arch_writer.data());
        break;
    case ARCHIVE_FILTER_XZ:
        ret = archive_write_add_filter_xz(arch_writer.data());
        break;
    case ARCHIVE_FILTER_LZMA:
        ret = archive_write_add_filter_lzma(arch_writer.data());
        break;
    case ARCHIVE_FILTER_NONE:
        ret = archive_write_add_filter_none(arch_writer.data());
        break;
    default:
        emit error(i18n("The compression type '%1' is not supported by Ark.", QLatin1String(archive_filter_name(arch_reader.data(), 0))));
        return false;
    }

    if (ret != ARCHIVE_OK) {
        emit error(xi18nc("@info", "Setting the compression method failed with the following error:"
                          "<nl/><message>%1</message>", QLatin1String(archive_error_string(arch_writer.data()))));
        return false;
    }

    ret = archive_write_open_fd(arch_writer.data(), tempFile.handle());
    if (ret != ARCHIVE_OK) {
        emit error(xi18nc("@info", "Opening the archive for writing failed with the following error:"
                          "<nl/><message>%1</message>", QLatin1String(archive_error_string(arch_writer.data()))));
        return false;
    }

    struct archive_entry *entry;

    // Copy old elements from previous archive to new archive.
    while (archive_read_next_header(arch_reader.data(), &entry) == ARCHIVE_OK) {

        const QString entryName = QFile::decodeName(archive_entry_pathname(entry));

        if (files.contains(entryName)) {
            archive_read_data_skip(arch_reader.data());
            qCDebug(ARK) << "Entry to be deleted, skipping" << entryName;
            emit entryRemoved(entryName);
            continue;
        }

        const int returnCode = archive_write_header(arch_writer.data(), entry);

        switch (returnCode) {
        case ARCHIVE_OK:
            // If the whole archive is extracted and the total filesize is
            // available, we use partial progress.
            copyData(QLatin1String(archive_entry_pathname(entry)), arch_reader.data(), arch_writer.data(), false);
            break;
        case ARCHIVE_FAILED:
        case ARCHIVE_FATAL:
            qCCritical(ARK) << "archive_write_header() has returned" << returnCode
                            << "with errno" << archive_errno(arch_writer.data());
            emit error(xi18nc("@info", "Compression failed while processing:<nl/>"
                              "<filename>%1</filename><nl/><nl/>Operation aborted.", entryName));
            return false;
        default:
            qCDebug(ARK) << "archive_writer_header() has returned" << returnCode
                         << "which will be ignored.";
            break;
        }
    }

    // In the success case, we need to manually close the archive_writer before
    // calling QSaveFile::commit(), otherwise the latter will close() the
    // file descriptor archive_writer is still working on.
    // TODO: We need to abstract this code better so that we only deal with one
    // object that manages both QSaveFile and ArchiveWriter.
    archive_write_close(arch_writer.data());
    tempFile.commit();

    return true;
}

void LibArchiveInterface::emitEntryFromArchiveEntry(struct archive_entry *aentry)
{
    ArchiveEntry e;

#ifdef _MSC_VER
    e[FileName] = QDir::fromNativeSeparators(QString::fromUtf16((ushort*)archive_entry_pathname_w(aentry)));
#else
    e[FileName] = QDir::fromNativeSeparators(QString::fromWCharArray(archive_entry_pathname_w(aentry)));
#endif
    e[InternalID] = e[FileName];

    const QString owner = QString::fromLatin1(archive_entry_uname(aentry));
    if (!owner.isEmpty()) {
        e[Owner] = owner;
    }

    const QString group = QString::fromLatin1(archive_entry_gname(aentry));
    if (!group.isEmpty()) {
        e[Group] = group;
    }

    e[Size] = (qlonglong)archive_entry_size(aentry);
    e[IsDirectory] = S_ISDIR(archive_entry_mode(aentry));

    if (archive_entry_symlink(aentry)) {
        e[Link] = QLatin1String( archive_entry_symlink(aentry) );
    }

    e[Timestamp] = QDateTime::fromTime_t(archive_entry_mtime(aentry));

    emit entry(e);
}

int LibArchiveInterface::extractionFlags() const
{
    int result = ARCHIVE_EXTRACT_TIME;
    result |= ARCHIVE_EXTRACT_SECURE_NODOTDOT;

    // TODO: Don't use arksettings here
    /*if ( ArkSettings::preservePerms() )
    {
        result &= ARCHIVE_EXTRACT_PERM;
    }

    if ( !ArkSettings::extractOverwrite() )
    {
        result &= ARCHIVE_EXTRACT_NO_OVERWRITE;
    }*/

    return result;
}

void LibArchiveInterface::copyData(const QString& filename, struct archive *dest, bool partialprogress)
{
    char buff[10240];
    ssize_t readBytes;
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    readBytes = file.read(buff, sizeof(buff));
    while (readBytes > 0) {
        archive_write_data(dest, buff, readBytes);
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

void LibArchiveInterface::copyData(const QString& filename, struct archive *source, struct archive *dest, bool partialprogress)
{
    char buff[10240];
    ssize_t readBytes;

    readBytes = archive_read_data(source, buff, sizeof(buff));
    while (readBytes > 0) {
        archive_write_data(dest, buff, readBytes);
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

// TODO: if we merge this with copyData(), we can pass more data
//       such as an fd to archive_read_disk_entry_from_file()
bool LibArchiveInterface::writeFile(const QString& fileName, struct archive* arch_writer)
{
    int header_response;

    const bool trailingSlash = fileName.endsWith(QLatin1Char( '/' ));

    // #191821: workDir must be used instead of QDir::current()
    //          so that symlinks aren't resolved automatically
    // TODO: this kind of call should be moved upwards in the
    //       class hierarchy to avoid code duplication
    const QString relativeName = m_workDir.relativeFilePath(fileName) + (trailingSlash ? QStringLiteral( "/" ) : QStringLiteral( "" ));

    // #253059: Even if we use archive_read_disk_entry_from_file,
    //          libarchive may have been compiled without HAVE_LSTAT,
    //          or something may have caused it to follow symlinks, in
    //          which case stat() will be called. To avoid this, we
    //          call lstat() ourselves.
    struct stat st;
    lstat(QFile::encodeName(fileName).constData(), &st);

    struct archive_entry *entry = archive_entry_new();
    archive_entry_set_pathname(entry, QFile::encodeName(relativeName).constData());
    archive_entry_copy_sourcepath(entry, QFile::encodeName(fileName).constData());
    archive_read_disk_entry_from_file(m_archiveReadDisk.data(), entry, -1, &st);

    qCDebug(ARK) << "Writing new entry " << archive_entry_pathname(entry);
    if ((header_response = archive_write_header(arch_writer, entry)) == ARCHIVE_OK) {
        // If the whole archive is extracted and the total filesize is
        // available, we use partial progress.
        copyData(fileName, arch_writer, false);
    } else {
        qCCritical(ARK) << "Writing header failed with error code " << header_response;
        qCCritical(ARK) << "Error while writing..." << archive_error_string(arch_writer) << "(error no =" << archive_errno(arch_writer) << ')';

        emit error(xi18nc("@info Error in a message box",
                          "Ark could not compress <filename>%1</filename>:<nl/>%2",
                          fileName,
                          QString::fromUtf8(archive_error_string(arch_writer))));

        archive_entry_free(entry);

        return false;
    }

    m_writtenFiles.push_back(relativeName);

    emitEntryFromArchiveEntry(entry);

    archive_entry_free(entry);

    return true;
}

#include "libarchivehandler.moc"
