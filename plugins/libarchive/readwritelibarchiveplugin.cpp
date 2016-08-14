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

ReadWriteLibarchivePlugin::ReadWriteLibarchivePlugin(QObject *parent, const QVariantList &args)
    : LibarchivePlugin(parent, args)
{
    qCDebug(ARK) << "Loaded libarchive read-write plugin";
}

ReadWriteLibarchivePlugin::~ReadWriteLibarchivePlugin()
{
}

bool ReadWriteLibarchivePlugin::addFiles(const QList<Archive::Entry*> &files, const Archive::Entry *destination, const CompressionOptions &options)
{
    qCDebug(ARK) << "Adding" << files.size() << "entries with CompressionOptions" << options;

    const bool creatingNewFile = !QFileInfo::exists(filename());

    m_writtenFiles.clear();

    if (!creatingNewFile && !initializeReader()) {
        return false;
    }

    if (!initializeWriter(creatingNewFile, options)) {
        return false;
    }

    // First write the new files.
    qCDebug(ARK) << "Writing new entries";
    int no_entries = 0;
    // Recreate destination directory structure.
    const QString destinationPath = (destination == Q_NULLPTR)
                                    ? QString()
                                    : destination->fullPath();

    foreach(Archive::Entry *selectedFile, files) {
        if (m_abortOperation) {
            break;
        }

        if (!writeFile(selectedFile->fullPath(), destinationPath)) {
            finish(false);
            return false;
        }
        no_entries++;

        // For directories, write all subfiles/folders.
        const QString &fullPath = selectedFile->fullPath();
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

                if (!writeFile(path, destinationPath)) {
                    finish(false);
                    return false;
                }
                no_entries++;
            }
        }
    }
    qCDebug(ARK) << "Added" << no_entries << "new entries to archive";

    bool isSuccessful = true;
    // If we have old archive entries.
    if (!creatingNewFile) {
        qCDebug(ARK) << "Copying any old entries";
        m_filePaths = m_writtenFiles;
        isSuccessful = processOldEntries(no_entries, Add);
        if (isSuccessful) {
            qCDebug(ARK) << "Added" << no_entries << "old entries to archive";
        }
        else {
            qCDebug(ARK) << "Adding entries failed";
        }
    }

    m_abortOperation = false;

    finish(isSuccessful);
    return isSuccessful;
}

bool ReadWriteLibarchivePlugin::moveFiles(const QList<Archive::Entry*> &files, Archive::Entry *destination, const CompressionOptions &options)
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
    int no_entries = 0;
    m_filePaths = entryFullPaths(entriesWithoutChildren(files));
    m_destination = destination;
    const bool isSuccessful = processOldEntries(no_entries, Move);
    if (isSuccessful) {
        qCDebug(ARK) << "Moved" << no_entries << "entries within archive";
    }
    else {
        qCDebug(ARK) << "Moving entries failed";
    }

    finish(isSuccessful);
    return isSuccessful;
}

bool ReadWriteLibarchivePlugin::copyFiles(const QList<Archive::Entry*> &files, Archive::Entry *destination, const CompressionOptions &options)
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
    int no_entries = 0;
    m_filePaths = entryFullPaths(entriesWithoutChildren(files));
    m_destination = destination;
    const bool isSuccessful = processOldEntries(no_entries, Copy);
    if (isSuccessful) {
        qCDebug(ARK) << "Copied" << no_entries << "entries within archive";
    }
    else {
        qCDebug(ARK) << "Copying entries failed";
    }

    finish(isSuccessful);
    return isSuccessful;
}

bool ReadWriteLibarchivePlugin::deleteFiles(const QList<Archive::Entry*> &files)
{
    qCDebug(ARK) << "Deleting" << files.size() << "entries";

    if (!initializeReader()) {
        return false;
    }

    if (!initializeWriter()) {
        return false;
    }

    // Copy old elements from previous archive to new archive.
    int no_entries = 0;
    m_filePaths = entryFullPaths(files);
    const bool isSuccessful = processOldEntries(no_entries, Delete);
    if (isSuccessful) {
        qCDebug(ARK) << "Removed" << no_entries << "entries from archive";
    }
    else {
        qCDebug(ARK) << "Removing entries failed";
    }

    finish(isSuccessful);
    return isSuccessful;
}

bool ReadWriteLibarchivePlugin::initializeWriter(const bool creatingNewFile, const CompressionOptions &options)
{
    // |tempFile| needs to be created before |arch_writer| so that when we go
    // out of scope in a `return false' case ArchiveWriteCustomDeleter is
    // called before destructor of QSaveFile (ie. we call archive_write_close()
    // before close()'ing the file descriptor).
    m_tempFile.setFileName(filename());
    if (!m_tempFile.open(QIODevice::WriteOnly | QIODevice::Unbuffered)) {
        emit error(xi18nc("@info", "Failed to create a temporary file to compress <filename>%1</filename>.", filename()));
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
    }
    else {
        if (!initializeWriterFilters()) {
            return false;
        }
    }

    if (archive_write_open_fd(m_archiveWriter.data(), m_tempFile.handle()) != ARCHIVE_OK) {
        emit error(xi18nc("@info", "Opening the archive for writing failed with the following error:"
                          "<nl/><message>%1</message>", QLatin1String(archive_error_string(m_archiveWriter.data()))));
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
#ifdef HAVE_LIBARCHIVE_3_2_0
        case ARCHIVE_FILTER_LZ4:
            ret = archive_write_add_filter_lz4(m_archiveWriter.data());
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
        emit error(xi18nc("@info", "Setting the compression method failed with the following error:<nl/><message>%1</message>",
                          QLatin1String(archive_error_string(m_archiveWriter.data()))));
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
#ifdef HAVE_LIBARCHIVE_3_2_0
        } else if (filename().right(3).toUpper() == QLatin1String("LZ4")) {
            qCDebug(ARK) << "Detected lz4 compression for new file";
            ret = archive_write_add_filter_lz4(m_archiveWriter.data());
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
        emit error(xi18nc("@info", "Setting the compression method failed with the following error:<nl/><message>%1</message>",
                          QLatin1String(archive_error_string(m_archiveWriter.data()))));
        return false;
    }

    // Set compression level if passed in CompressionOptions.
    if (options.contains(QStringLiteral("CompressionLevel"))) {
        qCDebug(ARK) << "Using compression level:" << options.value(QStringLiteral("CompressionLevel")).toString();
        ret = archive_write_set_filter_option(m_archiveWriter.data(), NULL, "compression-level", options.value(QStringLiteral("CompressionLevel")).toString().toUtf8());
        if (ret != ARCHIVE_OK) {
            qCWarning(ARK) << "Failed to set compression level";
            emit error(xi18nc("@info", "Setting the compression level failed with the following error:<nl/><message>%1</message>",
                              QLatin1String(archive_error_string(m_archiveWriter.data()))));
            return false;
        }
    }

    return true;
}

void ReadWriteLibarchivePlugin::finish(const bool isSuccessful)
{
    if (!isSuccessful) {
        m_tempFile.cancelWriting();
    }
    archive_write_close(m_archiveWriter.data());
    m_tempFile.commit();
}

bool ReadWriteLibarchivePlugin::processOldEntries(int &entriesCounter, OperationMode mode)
{
    struct archive_entry *entry;

    m_lastMovedFolder = QString();
    entriesCounter = 0;
    // If destination path doesn't contain a target entry name, we have to remember to include it
    // while moving or copying folder contents.
    int nameLength = 0;
    while ((mode != Add || !m_abortOperation) && archive_read_next_header(m_archiveReader.data(), &entry) == ARCHIVE_OK) {

        const QString file = QFile::decodeName(archive_entry_pathname(entry));

        if (mode == Move || mode == Copy) {
            QString newPathname;
            bool found = true;
            if (m_lastMovedFolder.count() > 0 && file.startsWith(m_lastMovedFolder)) {
                // Replace last moved or copied folder path with destination path.
                int charsCount = file.count() - m_lastMovedFolder.count();
                if (mode == Copy || m_filePaths.count() > 1) {
                    charsCount += nameLength;
                }
                newPathname = m_destination->fullPath() + file.right(charsCount);
            }
            else if (m_filePaths.contains(file)) {
                const QString name = file.split(QLatin1Char('/'), QString::SkipEmptyParts).last();
                if (mode == Copy || m_filePaths.count() > 1) {
                    newPathname = m_destination->fullPath() + name;
                }
                else {
                    // If the mode is set to Move and there is only one passed file in the list,
                    // we have to use destination as newPathname.
                    newPathname = m_destination->fullPath();
                }
                if (file.right(1) == QLatin1String("/")) {
                    nameLength = name.count() + 1; // plus slash
                    m_lastMovedFolder = file;
                }
                else {
                    nameLength = 0; // plus slash
                    m_lastMovedFolder = QString();
                }
            }
            else {
                found = false;
                m_lastMovedFolder = QString();
            }

            if (found) {
                if (mode == Copy) {
                    if (!writeEntry(entry)) {
                        return false;
                    }
                }
                else {
                    emit entryRemoved(file);
                }

                entriesCounter++;
                archive_entry_set_pathname(entry, newPathname.toUtf8());
            }
        }
        else if (m_filePaths.contains(file)) {
            archive_read_data_skip(m_archiveReader.data());
            switch (mode) {
                case Delete:
                    entriesCounter++;
                    emit entryRemoved(file);
                    break;

                case Add:
                    qCDebug(ARK) << file << "is already present in the new archive, skipping.";
                    break;

                default:
                    qCDebug(ARK) << "Mode" << mode << "is not considered for processing old libarchive entries";
                    Q_ASSERT(false);
            }
            continue;
        }

        if (writeEntry(entry)) {
            if (mode == Add) {
                entriesCounter++;
            }
            else if (mode == Move || mode == Copy) {
                emitEntryFromArchiveEntry(entry);
            }
        }
        else {
            return false;
        }
    }

    return true;
}

bool ReadWriteLibarchivePlugin::writeEntry(struct archive_entry *entry) {
    const int returnCode = archive_write_header(m_archiveWriter.data(), entry);
    const QString file = QFile::decodeName(archive_entry_pathname(entry));

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
            emit error(xi18nc("@info", "Compression failed while processing:<nl/>"
        "<filename>%1</filename><nl/><nl/>Operation aborted.", file));
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

    if ((header_response = archive_write_header(m_archiveWriter.data(), entry)) == ARCHIVE_OK) {
        // If the whole archive is extracted and the total filesize is
        // available, we use partial progress.
        copyData(absoluteFilename, m_archiveWriter.data(), false);
    } else {
        qCCritical(ARK) << "Writing header failed with error code " << header_response;
        qCCritical(ARK) << "Error while writing..." << archive_error_string(m_archiveWriter.data()) << "(error no =" << archive_errno(m_archiveWriter.data()) << ')';

        emit error(xi18nc("@info Error in a message box",
                          "Ark could not compress <filename>%1</filename>:<nl/>%2",
                          absoluteFilename,
                          QString::fromUtf8(archive_error_string(m_archiveWriter.data()))));

        archive_entry_free(entry);

        return false;
    }

    m_writtenFiles.push_back(destinationFilename);

    emitEntryFromArchiveEntry(entry);

    archive_entry_free(entry);

    return true;
}

#include "readwritelibarchiveplugin.moc"
