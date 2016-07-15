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

bool ReadWriteLibarchivePlugin::addFiles(const QList<Archive::Entry*>& files, const CompressionOptions& options)
{
    qCDebug(ARK) << "Adding" << files.size() << "entries with CompressionOptions" << options;

    const bool creatingNewFile = !QFileInfo::exists(filename());

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
    bool requiresExecutable = false;
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
        } else if (filename().right(2).toUpper() == QLatin1String(".Z")) {
            qCDebug(ARK) << "Detected compress (.Z) compression for new file";
            ret = archive_write_add_filter_compress(arch_writer.data());
        } else if (filename().right(2).toUpper() == QLatin1String("LZ")) {
            qCDebug(ARK) << "Detected lzip compression for new file";
            ret = archive_write_add_filter_lzip(arch_writer.data());
        } else if (filename().right(3).toUpper() == QLatin1String("LZO")) {
            qCDebug(ARK) << "Detected lzop compression for new file";
            ret = archive_write_add_filter_lzop(arch_writer.data());
        } else if (filename().right(3).toUpper() == QLatin1String("LRZ")) {
            qCDebug(ARK) << "Detected lrzip compression for new file";
            ret = archive_write_add_filter_lrzip(arch_writer.data());
            requiresExecutable = true;
#ifdef HAVE_LIBARCHIVE_3_2_0
        } else if (filename().right(3).toUpper() == QLatin1String("LZ4")) {
            qCDebug(ARK) << "Detected lz4 compression for new file";
            ret = archive_write_add_filter_lz4(arch_writer.data());
            requiresExecutable = true;
#endif
        } else if (filename().right(3).toUpper() == QLatin1String("TAR")) {
            qCDebug(ARK) << "Detected no compression for new file (pure tar)";
            ret = archive_write_add_filter_none(arch_writer.data());
        } else {
            qCDebug(ARK) << "Falling back to gzip";
            ret = archive_write_add_filter_gzip(arch_writer.data());
        }

        // Libarchive emits a warning for lrzip due to using external executable.
        if ((requiresExecutable && ret != ARCHIVE_WARN) ||
            (!requiresExecutable && ret != ARCHIVE_OK)) {
            emit error(xi18nc("@info", "Setting the compression method failed with the following error:<nl/><message>%1</message>",
                              QLatin1String(archive_error_string(arch_writer.data()))));
            return false;
        }

        // Set compression level if passed in CompressionOptions.
        if (options.contains(QStringLiteral("CompressionLevel"))) {
            qCDebug(ARK) << "Using compression level:" << options.value(QStringLiteral("CompressionLevel")).toString();
            ret = archive_write_set_filter_option(arch_writer.data(), NULL, "compression-level", options.value(QStringLiteral("CompressionLevel")).toString().toUtf8());
            if (ret != ARCHIVE_OK) {
                qCWarning(ARK) << "Failed to set compression level";
                emit error(xi18nc("@info", "Setting the compression level failed with the following error:<nl/><message>%1</message>",
                                  QLatin1String(archive_error_string(arch_writer.data()))));
                return false;
            }
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
        case ARCHIVE_FILTER_COMPRESS:
            ret = archive_write_add_filter_compress(arch_writer.data());
            break;
        case ARCHIVE_FILTER_LZIP:
            ret = archive_write_add_filter_lzip(arch_writer.data());
            break;
        case ARCHIVE_FILTER_LZOP:
            ret = archive_write_add_filter_lzop(arch_writer.data());
            break;
        case ARCHIVE_FILTER_LRZIP:
            ret = archive_write_add_filter_lrzip(arch_writer.data());
            requiresExecutable = true;
            break;
#ifdef HAVE_LIBARCHIVE_3_2_0
        case ARCHIVE_FILTER_LZ4:
            ret = archive_write_add_filter_lz4(arch_writer.data());
            requiresExecutable = true;
            break;
#endif
        case ARCHIVE_FILTER_NONE:
            ret = archive_write_add_filter_none(arch_writer.data());
            break;
        default:
            emit error(i18n("The compression type '%1' is not supported by Ark.",
                            QLatin1String(archive_filter_name(arch_reader.data(), 0))));
            return false;
        }

        // Libarchive emits a warning for lrzip due to using external executable.
        if ((requiresExecutable && ret != ARCHIVE_WARN) ||
            (!requiresExecutable && ret != ARCHIVE_OK)) {
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
    qCDebug(ARK) << "Writing new entries";
    int no_entries = 0;
    foreach(Archive::Entry *selectedFile, files) {

        if (m_abortOperation) {
            break;
        }

        if (!writeFile(selectedFile->property("fullPath").toString(), arch_writer.data())) {
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

                if (!writeFile(path, arch_writer.data())) {
                    return false;
                }
                no_entries++;
            }
        }
    }
    qCDebug(ARK) << "Added" << no_entries << "new entries to archive";

    struct archive_entry *entry;

    // If we have old archive entries.
    if (!creatingNewFile) {

        qCDebug(ARK) << "Copying any old entries";
        no_entries = 0;

        // Copy old entries from previous archive to new archive.
        while (!m_abortOperation && (archive_read_next_header(arch_reader.data(), &entry) == ARCHIVE_OK)) {

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
                no_entries++;
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
    int no_entries = 0;
    QStringList filePaths = entryFullPaths(files);
    while (archive_read_next_header(arch_reader.data(), &entry) == ARCHIVE_OK) {

        QString file = QFile::decodeName(archive_entry_pathname(entry));

        if (filePaths.contains(file)) {
            archive_read_data_skip(arch_reader.data());
            emit entryRemoved(file);
            no_entries++;
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
                              "<filename>%1</filename><nl/><nl/>Operation aborted.", file));
            return false;
        default:
            qCDebug(ARK) << "archive_writer_header() has returned" << returnCode
                         << "which will be ignored.";
            break;
        }
    }
    qCDebug(ARK) << "Removed" << no_entries << "entries from archive";

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

// TODO: if we merge this with copyData(), we can pass more data
//       such as an fd to archive_read_disk_entry_from_file()
bool ReadWriteLibarchivePlugin::writeFile(const QString& relativeName, struct archive* arch_writer)
{
    int header_response;
    const QString absoluteFilename = QFileInfo(relativeName).absoluteFilePath();

    // #253059: Even if we use archive_read_disk_entry_from_file,
    //          libarchive may have been compiled without HAVE_LSTAT,
    //          or something may have caused it to follow symlinks, in
    //          which case stat() will be called. To avoid this, we
    //          call lstat() ourselves.
    struct stat st;
    lstat(QFile::encodeName(absoluteFilename).constData(), &st);

    struct archive_entry *entry = archive_entry_new();
    archive_entry_set_pathname(entry, QFile::encodeName(relativeName).constData());
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

    m_writtenFiles.push_back(relativeName);

    emitEntryFromArchiveEntry(entry);

    archive_entry_free(entry);

    return true;
}

#include "readwritelibarchiveplugin.moc"
