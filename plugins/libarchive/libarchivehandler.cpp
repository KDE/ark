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

#include <config.h>

#include "libarchivehandler.h"
#include "kerfuffle/kerfuffle_export.h"
#include "kerfuffle/queries.h"

#include <archive.h>
#include <archive_entry.h>

#include <KDebug>
#include <KLocale>
#include <kde_file.h>

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QList>
#include <QStringList>

struct LibArchiveInterface::ArchiveReadCustomDeleter
{
    static inline void cleanup(struct archive *a)
    {
        if (a) {
            archive_read_finish(a);
        }
    }
};

struct LibArchiveInterface::ArchiveWriteCustomDeleter
{
    static inline void cleanup(struct archive *a)
    {
        if (a) {
            archive_write_finish(a);
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
    archive_read_disk_set_standard_lookup(m_archiveReadDisk.data());
}

LibArchiveInterface::~LibArchiveInterface()
{
}

bool LibArchiveInterface::list()
{
    kDebug();

    ArchiveRead arch_reader(archive_read_new());

    if (!(arch_reader.data())) {
        return false;
    }

    if (archive_read_support_compression_all(arch_reader.data()) != ARCHIVE_OK) {
        return false;
    }

    if (archive_read_support_format_all(arch_reader.data()) != ARCHIVE_OK) {
        return false;
    }

    if (archive_read_open_filename(arch_reader.data(), QFile::encodeName(filename()), 10240) != ARCHIVE_OK) {
        emit error(i18nc("@info", "Could not open the archive <filename>%1</filename>, libarchive cannot handle it.",
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
        emit error(i18nc("@info", "The archive reading failed with the following error: <message>%1</message>",
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

bool LibArchiveInterface::copyFiles(const QVariantList& files, const QString& destinationDirectory, ExtractionOptions options)
{
    kDebug() << "Changing current directory to " << destinationDirectory;
    QDir::setCurrent(destinationDirectory);

    const bool extractAll = files.isEmpty();
    const bool preservePaths = options.value(QLatin1String( "PreservePaths" )).toBool();

    QString rootNode = options.value(QLatin1String("RootNode"), QVariant()).toString();
    if ((!rootNode.isEmpty()) && (!rootNode.endsWith(QLatin1Char('/')))) {
        rootNode.append(QLatin1Char('/'));
    }

    ArchiveRead arch(archive_read_new());

    if (!(arch.data())) {
        return false;
    }

    if (archive_read_support_compression_all(arch.data()) != ARCHIVE_OK) {
        return false;
    }

    if (archive_read_support_format_all(arch.data()) != ARCHIVE_OK) {
        return false;
    }

    if (archive_read_open_filename(arch.data(), QFile::encodeName(filename()), 10240) != ARCHIVE_OK) {
        emit error(i18nc("@info", "Could not open the archive <filename>%1</filename>, libarchive cannot handle it.",
                   filename()));
        return false;
    }

    ArchiveWrite writer(archive_write_disk_new());
    if (!(writer.data())) {
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
            kDebug() << "For getting progress information, the archive will be listed once";
            m_emitNoEntries = true;
            list();
            m_emitNoEntries = false;
        }
        totalCount = m_cachedArchiveEntryCount;
    } else {
        totalCount = files.size();
    }

    m_currentExtractedFilesSize = 0;

    bool overwriteAll = false; // Whether to overwrite all files
    bool skipAll = false; // Whether to skip all files
    struct archive_entry *entry;

    QString fileBeingRenamed;

    while (archive_read_next_header(arch.data(), &entry) == ARCHIVE_OK) {
        fileBeingRenamed.clear();

        // retry with renamed entry, fire an overwrite query again
        // if the new entry also exists
    retry:
        const bool entryIsDir = S_ISDIR(archive_entry_mode(entry));

        //we skip directories if not preserving paths
        if (!preservePaths && entryIsDir) {
            archive_read_data_skip(arch.data());
            continue;
        }

        //entryName is the name inside the archive, full path
        QString entryName = QDir::fromNativeSeparators(QFile::decodeName(archive_entry_pathname(entry)));

        if (entryName.startsWith(QLatin1Char( '/' ))) {
            //for now we just can't handle absolute filenames in a tar archive.
            //TODO: find out what to do here!!
            emit error(i18n("This archive contains archive entries with absolute paths, which are not yet supported by ark."));

            return false;
        }

        if (files.contains(entryName) || entryName == fileBeingRenamed || extractAll) {
            // entryFI is the fileinfo pointing to where the file will be
            // written from the archive
            QFileInfo entryFI(entryName);
            //kDebug() << "setting path to " << archive_entry_pathname( entry );

            const QString fileWithoutPath(entryFI.fileName());

            //if we DON'T preserve paths, we cut the path and set the entryFI
            //fileinfo to the one without the path
            if (!preservePaths) {
                //empty filenames (ie dirs) should have been skipped already,
                //so asserting
                Q_ASSERT(!fileWithoutPath.isEmpty());

                archive_entry_copy_pathname(entry, QFile::encodeName(fileWithoutPath).constData());
                entryFI = QFileInfo(fileWithoutPath);

                //OR, if the commonBase has been set, then we remove this
                //common base from the filename
            } else if (!rootNode.isEmpty()) {
                kDebug() << "Removing" << rootNode << "from" << entryName;

                const QString truncatedFilename(entryName.remove(0, rootNode.size()));
                archive_entry_copy_pathname(entry, QFile::encodeName(truncatedFilename).constData());

                entryFI = QFileInfo(truncatedFilename);
            }

            //now check if the file about to be written already exists
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

            //if there is an already existing directory:
            if (entryIsDir && entryFI.exists()) {
                if (entryFI.isWritable()) {
                    kDebug(1601) << "Warning, existing, but writable dir";
                } else {
                    kDebug(1601) << "Warning, existing, but non-writable dir. skipping";
                    archive_entry_clear(entry);
                    archive_read_data_skip(arch.data());
                    continue;
                }
            }

            int header_response;
            kDebug() << "Writing " << fileWithoutPath << " to " << archive_entry_pathname(entry);
            if ((header_response = archive_write_header(writer.data(), entry)) == ARCHIVE_OK) {
                //if the whole archive is extracted and the total filesize is
                //available, we use partial progress
                copyData(arch.data(), writer.data(), (extractAll && m_extractedFilesSize));
            } else if (header_response == ARCHIVE_WARN) {
                kDebug() << "Warning while writing " << entryName;
            } else {
                kDebug() << "Writing header failed with error code " << header_response
                << "While attempting to write " << entryName;
            }

            //if we only partially extract the archive and the number of
            //archive entries is available we use a simple progress based on
            //number of items extracted
            if (!extractAll && m_cachedArchiveEntryCount) {
                ++entryNr;
                emit progress(float(entryNr) / totalCount);
            }
            archive_entry_clear(entry);
        } else {
            archive_read_data_skip(arch.data());
        }
    }

    return archive_read_close(arch.data()) == ARCHIVE_OK;
}

bool LibArchiveInterface::addFiles(const QStringList& files, const CompressionOptions& options)
{
    const bool creatingNewFile = !QFileInfo(filename()).exists();
    const QString tempFilename = filename() + QLatin1String( ".arkWriting" );
    const QString globalWorkDir = options.value(QLatin1String( "GlobalWorkDir" )).toString();

    if (!globalWorkDir.isEmpty()) {
        kDebug() << "GlobalWorkDir is set, changing dir to " << globalWorkDir;
        m_workDir.setPath(globalWorkDir);
        QDir::setCurrent(globalWorkDir);
    }

    m_writtenFiles.clear();

    ArchiveRead arch_reader;
    if (!creatingNewFile) {
        arch_reader.reset(archive_read_new());
        if (!(arch_reader.data())) {
            emit error(i18n("The archive reader could not be initialized."));
            return false;
        }

        if (archive_read_support_compression_all(arch_reader.data()) != ARCHIVE_OK) {
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

    ArchiveWrite arch_writer(archive_write_new());
    if (!(arch_writer.data())) {
        emit error(i18n("The archive writer could not be initialized."));
        return false;
    }

    //pax_restricted is the libarchive default, let's go with that.
    archive_write_set_format_pax_restricted(arch_writer.data());

    int ret;
    if (creatingNewFile) {
        if (filename().right(2).toUpper() == QLatin1String( "GZ" )) {
            kDebug() << "Detected gzip compression for new file";
            ret = archive_write_set_compression_gzip(arch_writer.data());
        } else if (filename().right(3).toUpper() == QLatin1String( "BZ2" )) {
            kDebug() << "Detected bzip2 compression for new file";
            ret = archive_write_set_compression_bzip2(arch_writer.data());
#ifdef HAVE_LIBARCHIVE_XZ_SUPPORT
        } else if (filename().right(2).toUpper() == QLatin1String( "XZ" )) {
            kDebug() << "Detected xz compression for new file";
            ret = archive_write_set_compression_xz(arch_writer.data());
#endif
#ifdef HAVE_LIBARCHIVE_LZMA_SUPPORT
        } else if (filename().right(4).toUpper() == QLatin1String( "LZMA" )) {
            kDebug() << "Detected lzma compression for new file";
            ret = archive_write_set_compression_lzma(arch_writer.data());
#endif
        } else if (filename().right(3).toUpper() == QLatin1String( "TAR" )) {
            kDebug() << "Detected no compression for new file (pure tar)";
            ret = archive_write_set_compression_none(arch_writer.data());
        } else {
            kDebug() << "Falling back to gzip";
            ret = archive_write_set_compression_gzip(arch_writer.data());
        }

        if (ret != ARCHIVE_OK) {
            emit error(i18nc("@info", "Setting the compression method failed with the following error: <message>%1</message>",
                       QLatin1String(archive_error_string(arch_writer.data()))));

            return false;
        }
    } else {
        switch (archive_compression(arch_reader.data())) {
        case ARCHIVE_COMPRESSION_GZIP:
            ret = archive_write_set_compression_gzip(arch_writer.data());
            break;
        case ARCHIVE_COMPRESSION_BZIP2:
            ret = archive_write_set_compression_bzip2(arch_writer.data());
            break;
#ifdef HAVE_LIBARCHIVE_XZ_SUPPORT
        case ARCHIVE_COMPRESSION_XZ:
            ret = archive_write_set_compression_xz(arch_writer.data());
            break;
#endif
#ifdef HAVE_LIBARCHIVE_LZMA_SUPPORT
        case ARCHIVE_COMPRESSION_LZMA:
            ret = archive_write_set_compression_lzma(arch_writer.data());
            break;
#endif
        case ARCHIVE_COMPRESSION_NONE:
            ret = archive_write_set_compression_none(arch_writer.data());
            break;
        default:
            emit error(i18n("The compression type '%1' is not supported by Ark.", QLatin1String(archive_compression_name(arch_reader.data()))));
            return false;
        }

        if (ret != ARCHIVE_OK) {
            emit error(i18nc("@info", "Setting the compression method failed with the following error: <message>%1</message>", QLatin1String(archive_error_string(arch_writer.data()))));
            return false;
        }
    }

    ret = archive_write_open_filename(arch_writer.data(), QFile::encodeName(tempFilename));
    if (ret != ARCHIVE_OK) {
        emit error(i18nc("@info", "Opening the archive for writing failed with the following error: <message>%1</message>", QLatin1String(archive_error_string(arch_writer.data()))));
        return false;
    }

    //**************** first write the new files
    foreach(const QString& selectedFile, files) {
        bool success;

        success = writeFile(selectedFile, arch_writer.data());

        if (!success) {
            QFile::remove(tempFilename);
            return false;
        }

        if (QFileInfo(selectedFile).isDir()) {
            QDirIterator it(selectedFile,
                            QDir::AllEntries | QDir::Readable |
                            QDir::Hidden | QDir::NoDotAndDotDot,
                            QDirIterator::Subdirectories);

            while (it.hasNext()) {
                const QString path = it.next();

                if ((it.fileName() == QLatin1String("..")) ||
                    (it.fileName() == QLatin1String("."))) {
                    continue;
                }

                success = writeFile(path +
                                    (it.fileInfo().isDir() ? QLatin1String( "/" ) : QLatin1String( "" )),
                                    arch_writer.data());

                if (!success) {
                    QFile::remove(tempFilename);
                    return false;
                }
            }
        }
    }

    struct archive_entry *entry;

    //and if we have old elements...
    if (!creatingNewFile) {
        //********** copy old elements from previous archive to new archive
        while (archive_read_next_header(arch_reader.data(), &entry) == ARCHIVE_OK) {
            if (m_writtenFiles.contains(QFile::decodeName(archive_entry_pathname(entry)))) {
                archive_read_data_skip(arch_reader.data());
                kDebug() << "Entry already existing, will be refresh: ===> " << archive_entry_pathname(entry);
                continue;
            }

            int header_response;
            //kDebug() << "Writing entry " << fn;
            if ((header_response = archive_write_header(arch_writer.data(), entry)) == ARCHIVE_OK) {
                //if the whole archive is extracted and the total filesize is
                //available, we use partial progress
                copyData(arch_reader.data(), arch_writer.data(), false);
            } else {
                kDebug() << "Writing header failed with error code " << header_response;
                QFile::remove(tempFilename);
                return false;
            }

            archive_entry_clear(entry);
        }

        //everything seems OK, so we remove the source file and replace it with
        //the new one.
        //TODO: do some extra checks to see if this is really OK
        QFile::remove(filename());
    }

    QFile::rename(tempFilename, filename());

    return true;
}

bool LibArchiveInterface::deleteFiles(const QVariantList& files)
{
    const QString tempFilename = filename() + QLatin1String( ".arkWriting" );

    ArchiveRead arch_reader(archive_read_new());
    if (!(arch_reader.data())) {
        emit error(i18n("The archive reader could not be initialized."));
        return false;
    }

    if (archive_read_support_compression_all(arch_reader.data()) != ARCHIVE_OK) {
        return false;
    }

    if (archive_read_support_format_all(arch_reader.data()) != ARCHIVE_OK) {
        return false;
    }

    if (archive_read_open_filename(arch_reader.data(), QFile::encodeName(filename()), 10240) != ARCHIVE_OK) {
        emit error(i18n("The source file could not be read."));
        return false;
    }

    ArchiveWrite arch_writer(archive_write_new());
    if (!(arch_writer.data())) {
        emit error(i18n("The archive writer could not be initialized."));
        return false;
    }

    //pax_restricted is the libarchive default, let's go with that.
    archive_write_set_format_pax_restricted(arch_writer.data());

    int ret;
    switch (archive_compression(arch_reader.data())) {
    case ARCHIVE_COMPRESSION_GZIP:
        ret = archive_write_set_compression_gzip(arch_writer.data());
        break;
    case ARCHIVE_COMPRESSION_BZIP2:
        ret = archive_write_set_compression_bzip2(arch_writer.data());
        break;
#ifdef HAVE_LIBARCHIVE_XZ_SUPPORT
    case ARCHIVE_COMPRESSION_XZ:
        ret = archive_write_set_compression_xz(arch_writer.data());
        break;
#endif
#ifdef HAVE_LIBARCHIVE_LZMA_SUPPORT
    case ARCHIVE_COMPRESSION_LZMA:
        ret = archive_write_set_compression_lzma(arch_writer.data());
        break;
#endif
    case ARCHIVE_COMPRESSION_NONE:
        ret = archive_write_set_compression_none(arch_writer.data());
        break;
    default:
        emit error(i18n("The compression type '%1' is not supported by Ark.", QLatin1String(archive_compression_name(arch_reader.data()))));
        return false;
    }

    if (ret != ARCHIVE_OK) {
        emit error(i18nc("@info", "Setting the compression method failed with the following error: <message>%1</message>", QLatin1String(archive_error_string(arch_writer.data()))));
        return false;
    }

    ret = archive_write_open_filename(arch_writer.data(), QFile::encodeName(tempFilename));
    if (ret != ARCHIVE_OK) {
        emit error(i18nc("@info", "Opening the archive for writing failed with the following error: <message>%1</message>", QLatin1String(archive_error_string(arch_writer.data()))));
        return false;
    }

    struct archive_entry *entry;

    //********** copy old elements from previous archive to new archive
    while (archive_read_next_header(arch_reader.data(), &entry) == ARCHIVE_OK) {
        if (files.contains(QFile::decodeName(archive_entry_pathname(entry)))) {
            archive_read_data_skip(arch_reader.data());
            kDebug() << "Entry to be deleted, skipping"
                     << archive_entry_pathname(entry);
            emit entryRemoved(QFile::decodeName(archive_entry_pathname(entry)));
            continue;
        }

        int header_response;
        //kDebug() << "Writing entry " << fn;
        if ((header_response = archive_write_header(arch_writer.data(), entry)) == ARCHIVE_OK) {
            //if the whole archive is extracted and the total filesize is
            //available, we use partial progress
            copyData(arch_reader.data(), arch_writer.data(), false);
        } else {
            kDebug() << "Writing header failed with error code " << header_response;
            return false;
        }
    }

    //everything seems OK, so we remove the source file and replace it with
    //the new one.
    //TODO: do some extra checks to see if this is really OK
    QFile::remove(filename());
    QFile::rename(tempFilename, filename());

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

    const QString owner = QString::fromAscii(archive_entry_uname(aentry));
    if (!owner.isEmpty()) {
        e[Owner] = owner;
    }

    const QString group = QString::fromAscii(archive_entry_gname(aentry));
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
        /* int writeBytes = */
        archive_write_data(dest, buff, readBytes);
        if (archive_errno(dest) != ARCHIVE_OK) {
            kDebug() << "Error while writing..." << archive_error_string(dest) << "(error nb =" << archive_errno(dest) << ')';
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

void LibArchiveInterface::copyData(struct archive *source, struct archive *dest, bool partialprogress)
{
    char buff[10240];
    ssize_t readBytes;

    readBytes = archive_read_data(source, buff, sizeof(buff));
    while (readBytes > 0) {
        /* int writeBytes = */
        archive_write_data(dest, buff, readBytes);
        if (archive_errno(dest) != ARCHIVE_OK) {
            kDebug() << "Error while extracting..." << archive_error_string(dest) << "(error nb =" << archive_errno(dest) << ')';
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
    const QString relativeName = m_workDir.relativeFilePath(fileName) + (trailingSlash ? QLatin1String( "/" ) : QLatin1String( "" ));

    // #253059: Even if we use archive_read_disk_entry_from_file,
    //          libarchive may have been compiled without HAVE_LSTAT,
    //          or something may have caused it to follow symlinks, in
    //          which case stat() will be called. To avoid this, we
    //          call lstat() ourselves.
    KDE_struct_stat st;
    KDE_lstat(QFile::encodeName(fileName).constData(), &st);

    struct archive_entry *entry = archive_entry_new();
    archive_entry_set_pathname(entry, QFile::encodeName(relativeName).constData());
    archive_entry_copy_sourcepath(entry, QFile::encodeName(fileName).constData());
    archive_read_disk_entry_from_file(m_archiveReadDisk.data(), entry, -1, &st);

    kDebug() << "Writing new entry " << archive_entry_pathname(entry);
    if ((header_response = archive_write_header(arch_writer, entry)) == ARCHIVE_OK) {
        //if the whole archive is extracted and the total filesize is
        //available, we use partial progress
        copyData(fileName, arch_writer, false);
    } else {
        kDebug() << "Writing header failed with error code " << header_response;
        kDebug() << "Error while writing..." << archive_error_string(arch_writer) << "(error nb =" << archive_errno(arch_writer) << ')';

        emit error(i18nc("@info Error in a message box",
                    "Ark could not compress <filename>%1</filename>:<nl/>%2",
                    fileName,
                    QLatin1String(archive_error_string(arch_writer))));

        archive_entry_free(entry);

        return false;
    }

    m_writtenFiles.push_back(relativeName);

    emitEntryFromArchiveEntry(entry);

    archive_entry_free(entry);

    return true;
}

KERFUFFLE_EXPORT_PLUGIN(LibArchiveInterface)

#include "libarchivehandler.moc"
