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

#include "readwritelibarchiveplugin.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QDirIterator>
#include <QSaveFile>

K_PLUGIN_FACTORY_WITH_JSON(ReadWriteLibarchivePluginFactory, "kerfuffle_libarchive.json", registerPlugin<ReadWriteLibarchivePlugin>();)

ReadWriteLibarchivePlugin::ReadWriteLibarchivePlugin(QObject *parent, const QVariantList & args)
    : LibarchivePlugin(parent, args)
{
    qCDebug(ARK) << "Loaded libarchive read-write plugin";
}

ReadWriteLibarchivePlugin::~ReadWriteLibarchivePlugin()
{
}

bool ReadWriteLibarchivePlugin::addFiles(const QList<Archive::Entry*> &files, const Archive::Entry *destination, const QString &tempDirPath, const CompressionOptions& options)
{
    qCDebug(ARK) << "Adding" << files.size() << "entries with CompressionOptions" << options;

    const bool creatingNewFile = !QFileInfo::exists(filename());

    m_writtenFiles.clear();

    ArchiveRead arch_reader;
    if (!creatingNewFile) {
        arch_reader.reset(archive_read_new());
        if (!initializeReader(arch_reader)) {
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

    if (creatingNewFile) {
        if (!initializeNewFileWriter(arch_writer, options)) {
            return false;
        }
    } else {
        if (!initializeWriter(arch_writer, arch_reader)) {
            return false;
        }
    }

    if (archive_write_open_fd(arch_writer.data(), tempFile.handle()) != ARCHIVE_OK) {
        emit error(xi18nc("@info", "Opening the archive for writing failed with the following error:<nl/><message>%1</message>",
                          QLatin1String(archive_error_string(arch_writer.data()))));
        return false;
    }

    // First write the new files.
    qCDebug(ARK) << "Writing new entries";
    int no_entries = 0;
    // Recreate destination directory structure.
    const QString destinationPath = destination->property("fullPath").toString();
    qCDebug(ARK) << destinationPath.split(QLatin1Char('/'));
    foreach(const QString& directory, destinationPath.split(QLatin1Char('/'))) {
        if (!directory.isEmpty()) {
            qCDebug(ARK) << directory;
            if (!writeFile(directory, QString(), arch_writer.data())) {
                return false;
            }
            no_entries++;
        }
    }
    foreach(Archive::Entry *selectedFile, files) {

        if (m_abortOperation) {
            break;
        }

        if (!writeFile(selectedFile->property("fullPath").toString(), destinationPath, arch_writer.data())) {
            return false;
        }
        no_entries++;

        // For directories, write all subfiles/folders.
        const QString &fullPath = selectedFile->property("fullPath").toString();
        if (QFileInfo(fullPath).isDir()) {
            QDirIterator it(fullPath,
                            QDir::AllEntries | QDir::Readable |
                            QDir::Hidden | QDir::NoDotAndDotDot,
                            QDirIterator::Subdirectories);

            while (!m_abortOperation && it.hasNext()) {
                QString path = it.next();

                if ((it.fileName() == QLatin1String("..")) ||
                    (it.fileName() == QLatin1String("."))) {
                    continue;
                }

                const bool isRealDir = it.fileInfo().isDir() && !it.fileInfo().isSymLink();

                if (isRealDir) {
                    path.append(QLatin1Char('/'));
                }

                if (!writeFile(path, destinationPath, arch_writer.data())) {
                    return false;
                }
                no_entries++;
            }
        }
    }
    qCDebug(ARK) << "Added" << no_entries << "new entries to archive";

    // If we have old archive entries.
    if (!creatingNewFile) {

        qCDebug(ARK) << "Copying any old entries";
        if (!copyOldEntries(arch_writer, arch_reader, m_writtenFiles, no_entries)) {
            QFile::remove(tempFile.fileName());
            return false;
        }

        qCDebug(ARK) << "Added" << no_entries << "old entries to archive";
    }

    m_abortOperation = false;

    // In the success case, we need to manually close the archive_writer before
    // calling QSaveFile::commit(), otherwise the latter will close() the
    // file descriptor archive_writer is still working on.
    // TODO: We need to abstract this code better so that we only deal with one
    // object that manages both QSaveFile and ArchiveWriter.
    archive_write_close(arch_writer.data());
    tempFile.commit();

    return true;
}

bool ReadWriteLibarchivePlugin::deleteFiles(const QList<Archive::Entry*>& files)
{
   qCDebug(ARK) << "Deleting" << files.size() << "entries";

    ArchiveRead arch_reader(archive_read_new());
    if (!initializeReader(arch_reader)) {
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

    if (!initializeWriter(arch_writer, arch_reader)) {
        return false;
    }

    if (archive_write_open_fd(arch_writer.data(), tempFile.handle()) != ARCHIVE_OK) {
        emit error(xi18nc("@info", "Opening the archive for writing failed with the following error:"
                          "<nl/><message>%1</message>", QLatin1String(archive_error_string(arch_writer.data()))));
        return false;
    }

    // Copy old elements from previous archive to new archive.
    int no_entries = 0;
    QStringList filePaths = entryFullPaths(files);
    copyOldEntries(arch_writer, arch_reader, filePaths, no_entries, true);
    qCDebug(ARK) << "Removed" << no_entries << "entries from archive";

    // In the success case, we need to manually close the archive_writer before
    // calling QSaveFile::commit(), otherwise the latter will close() the
    // file descriptor archive_writer is still working on.
    // TODO: We need to abstract this code better so that we only deal with one
    // object that manages both QSaveFile and ArchiveWriter.
    archive_write_close(arch_writer.data());
    tempFile.commit();

    return true;
}

bool ReadWriteLibarchivePlugin::initializeNewFileWriter(const LibarchivePlugin::ArchiveWrite &archiveWrite, const CompressionOptions &options)
{
    int ret;
    bool requiresExecutable = false;
    if (filename().right(2).toUpper() == QLatin1String("GZ")) {
        qCDebug(ARK) << "Detected gzip compression for new file";
        ret = archive_write_add_filter_gzip(archiveWrite.data());
    } else if (filename().right(3).toUpper() == QLatin1String("BZ2")) {
        qCDebug(ARK) << "Detected bzip2 compression for new file";
        ret = archive_write_add_filter_bzip2(archiveWrite.data());
    } else if (filename().right(2).toUpper() == QLatin1String("XZ")) {
        qCDebug(ARK) << "Detected xz compression for new file";
        ret = archive_write_add_filter_xz(archiveWrite.data());
    } else if (filename().right(4).toUpper() == QLatin1String("LZMA")) {
        qCDebug(ARK) << "Detected lzma compression for new file";
        ret = archive_write_add_filter_lzma(archiveWrite.data());
    } else if (filename().right(2).toUpper() == QLatin1String(".Z")) {
        qCDebug(ARK) << "Detected compress (.Z) compression for new file";
        ret = archive_write_add_filter_compress(archiveWrite.data());
    } else if (filename().right(2).toUpper() == QLatin1String("LZ")) {
        qCDebug(ARK) << "Detected lzip compression for new file";
        ret = archive_write_add_filter_lzip(archiveWrite.data());
    } else if (filename().right(3).toUpper() == QLatin1String("LZO")) {
        qCDebug(ARK) << "Detected lzop compression for new file";
        ret = archive_write_add_filter_lzop(archiveWrite.data());
    } else if (filename().right(3).toUpper() == QLatin1String("LRZ")) {
        qCDebug(ARK) << "Detected lrzip compression for new file";
        ret = archive_write_add_filter_lrzip(archiveWrite.data());
        requiresExecutable = true;
#ifdef HAVE_LIBARCHIVE_3_2_0
        } else if (filename().right(3).toUpper() == QLatin1String("LZ4")) {
            qCDebug(ARK) << "Detected lz4 compression for new file";
            ret = archive_write_add_filter_lz4(archiveWrite.data());
#endif
    } else if (filename().right(3).toUpper() == QLatin1String("TAR")) {
        qCDebug(ARK) << "Detected no compression for new file (pure tar)";
        ret = archive_write_add_filter_none(archiveWrite.data());
    } else {
        qCDebug(ARK) << "Falling back to gzip";
        ret = archive_write_add_filter_gzip(archiveWrite.data());
    }

    // Libarchive emits a warning for lrzip due to using external executable.
    if ((requiresExecutable && ret != ARCHIVE_WARN) ||
        (!requiresExecutable && ret != ARCHIVE_OK)) {
        emit error(xi18nc("@info", "Setting the compression method failed with the following error:<nl/><message>%1</message>",
                          QLatin1String(archive_error_string(archiveWrite.data()))));
        return false;
    }

    // Set compression level if passed in CompressionOptions.
    if (options.contains(QStringLiteral("CompressionLevel"))) {
        qCDebug(ARK) << "Using compression level:" << options.value(QStringLiteral("CompressionLevel")).toString();
        ret = archive_write_set_filter_option(archiveWrite.data(), NULL, "compression-level", options.value(QStringLiteral("CompressionLevel")).toString().toUtf8());
        if (ret != ARCHIVE_OK) {
            qCWarning(ARK) << "Failed to set compression level";
            emit error(xi18nc("@info", "Setting the compression level failed with the following error:<nl/><message>%1</message>",
                              QLatin1String(archive_error_string(archiveWrite.data()))));
            return false;
        }
    }

    return true;
}

bool ReadWriteLibarchivePlugin::initializeWriter(const ArchiveWrite &archiveWrite, const ArchiveRead &archiveRead)
{
    int ret;
    bool requiresExecutable = false;
    switch (archive_filter_code(archiveRead.data(), 0)) {
        case ARCHIVE_FILTER_GZIP:
            ret = archive_write_add_filter_gzip(archiveWrite.data());
            break;
        case ARCHIVE_FILTER_BZIP2:
            ret = archive_write_add_filter_bzip2(archiveWrite.data());
            break;
        case ARCHIVE_FILTER_XZ:
            ret = archive_write_add_filter_xz(archiveWrite.data());
            break;
        case ARCHIVE_FILTER_LZMA:
            ret = archive_write_add_filter_lzma(archiveWrite.data());
            break;
        case ARCHIVE_FILTER_COMPRESS:
            ret = archive_write_add_filter_compress(archiveWrite.data());
            break;
        case ARCHIVE_FILTER_LZIP:
            ret = archive_write_add_filter_lzip(archiveWrite.data());
            break;
        case ARCHIVE_FILTER_LZOP:
            ret = archive_write_add_filter_lzop(archiveWrite.data());
            break;
        case ARCHIVE_FILTER_LRZIP:
            ret = archive_write_add_filter_lrzip(archiveWrite.data());
            requiresExecutable = true;
            break;
#ifdef HAVE_LIBARCHIVE_3_2_0
        case ARCHIVE_FILTER_LZ4:
            ret = archive_write_add_filter_lz4(archiveWrite.data());
            break;
#endif
        case ARCHIVE_FILTER_NONE:
            ret = archive_write_add_filter_none(archiveWrite.data());
            break;
        default:
            emit error(i18n("The compression type '%1' is not supported by Ark.",
                            QLatin1String(archive_filter_name(archiveRead.data(), 0))));
            return false;
    }

    // Libarchive emits a warning for lrzip due to using external executable.
    if ((requiresExecutable && ret != ARCHIVE_WARN) ||
        (!requiresExecutable && ret != ARCHIVE_OK)) {
        emit error(xi18nc("@info", "Setting the compression method failed with the following error:<nl/><message>%1</message>",
                          QLatin1String(archive_error_string(archiveWrite.data()))));
        return false;
    }

    return true;
}

bool ReadWriteLibarchivePlugin::copyOldEntries(const LibarchivePlugin::ArchiveWrite &archiveWrite,
                                               const LibarchivePlugin::ArchiveRead &archiveRead,
                                               const QStringList &filePaths,
                                               int &entriesCounter,
                                               bool deleteMode)
{
    struct archive_entry *entry;

    entriesCounter = 0;
    while ((deleteMode || !m_abortOperation) && archive_read_next_header(archiveRead.data(), &entry) == ARCHIVE_OK) {

        QString file = QFile::decodeName(archive_entry_pathname(entry));

        if (filePaths.contains(file)) {
            archive_read_data_skip(archiveRead.data());
            if (deleteMode) {
                emit entryRemoved(file);
                entriesCounter++;
            }
            else {
                qCDebug(ARK) << file << "is already present in the new archive, skipping.";
            }
            continue;
        }

        const int returnCode = archive_write_header(archiveWrite.data(), entry);

        switch (returnCode) {
            case ARCHIVE_OK:
                // If the whole archive is extracted and the total filesize is
                // available, we use partial progress.
                copyData(QLatin1String(archive_entry_pathname(entry)), archiveRead.data(), archiveWrite.data(), false);
                if (!deleteMode) {
                    entriesCounter++;
                }
                break;
            case ARCHIVE_FAILED:
            case ARCHIVE_FATAL:
                qCCritical(ARK) << "archive_write_header() has returned" << returnCode
                                << "with errno" << archive_errno(archiveWrite.data());
                emit error(xi18nc("@info", "Compression failed while processing:<nl/>"
            "<filename>%1</filename><nl/><nl/>Operation aborted.", file));
                return false;
            default:
                qCDebug(ARK) << "archive_writer_header() has returned" << returnCode
                             << "which will be ignored.";
                break;
        }
        if (!deleteMode) {
            archive_entry_clear(entry);
        }
    }

    return true;
}

// TODO: if we merge this with copyData(), we can pass more data
//       such as an fd to archive_read_disk_entry_from_file()
bool ReadWriteLibarchivePlugin::writeFile(const QString& relativeName, const QString& destination, struct archive* arch_writer)
{
    int header_response;
    const QString absoluteFilename = QFileInfo(relativeName).absoluteFilePath();
    const QString destinationFilename = destination + relativeName;

    // #253059: Even if we use archive_read_disk_entry_from_file,
    //          libarchive may have been compiled without HAVE_LSTAT,
    //          or something may have caused it to follow symlinks, in
    //          which case stat() will be called. To avoid this, we
    //          call lstat() ourselves.
    struct stat st;
    lstat(QFile::encodeName(absoluteFilename).constData(), &st);

    struct archive_entry *entry = archive_entry_new();
    archive_entry_set_pathname(entry, QFile::encodeName(destinationFilename).constData());
    archive_entry_copy_sourcepath(entry, QFile::encodeName(absoluteFilename).constData());
    archive_read_disk_entry_from_file(m_archiveReadDisk.data(), entry, -1, &st);

    if ((header_response = archive_write_header(arch_writer, entry)) == ARCHIVE_OK) {
        // If the whole archive is extracted and the total filesize is
        // available, we use partial progress.
        copyData(absoluteFilename, arch_writer, false);
    } else {
        qCCritical(ARK) << "Writing header failed with error code " << header_response;
        qCCritical(ARK) << "Error while writing..." << archive_error_string(arch_writer) << "(error no =" << archive_errno(arch_writer) << ')';

        emit error(xi18nc("@info Error in a message box",
                          "Ark could not compress <filename>%1</filename>:<nl/>%2",
                          absoluteFilename,
                          QString::fromUtf8(archive_error_string(arch_writer))));

        archive_entry_free(entry);

        return false;
    }

    m_writtenFiles.push_back(destinationFilename);

    emitEntryFromArchiveEntry(entry);

    archive_entry_free(entry);

    return true;
}

#include "readwritelibarchiveplugin.moc"
