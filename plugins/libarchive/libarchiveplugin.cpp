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

#include "libarchiveplugin.h"
#include "kerfuffle/queries.h"

#include <KLocalizedString>

#include <QDirIterator>

LibarchivePlugin::LibarchivePlugin(QObject *parent, const QVariantList & args)
    : ReadWriteArchiveInterface(parent, args)
    , m_archiveReadDisk(archive_read_disk_new())
    , m_abortOperation(false)
    , m_cachedArchiveEntryCount(0)
    , m_emitNoEntries(false)
    , m_extractedFilesSize(0)
{
    qCDebug(ARK) << "Initializing libarchive plugin";
    archive_read_disk_set_standard_lookup(m_archiveReadDisk.data());
}

LibarchivePlugin::~LibarchivePlugin()
{
}

bool LibarchivePlugin::list()
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
        emit error(i18nc("@info", "Could not open the archive."));
        return false;
    }

    qDebug(ARK) << "Detected compression filter:" << archive_filter_name(arch_reader.data(), 0);

    m_cachedArchiveEntryCount = 0;
    m_extractedFilesSize = 0;

    struct archive_entry *aentry;
    int result = ARCHIVE_RETRY;

    bool firstEntry = true;
    while (!m_abortOperation && (result = archive_read_next_header(arch_reader.data(), &aentry)) == ARCHIVE_OK) {

        if (firstEntry) {
            qDebug(ARK) << "Detected format for first entry:" << archive_format_name(arch_reader.data());
            firstEntry = false;
        }

        if (!m_emitNoEntries) {
            emitEntryFromArchiveEntry(aentry);
        }

        m_extractedFilesSize += (qlonglong)archive_entry_size(aentry);

        m_cachedArchiveEntryCount++;
        archive_read_data_skip(arch_reader.data());
    }
    m_abortOperation = false;

    if (result != ARCHIVE_EOF) {
        const QString errorString = QLatin1String(archive_error_string(arch_reader.data()));
        // FIXME: what about the other archive_error_string() calls? Do they also happen to return empty strings?
        emit error(errorString.isEmpty() ? i18nc("@info", "Could not read until the end of the archive") : errorString);
        return false;
    }

    return archive_read_close(arch_reader.data()) == ARCHIVE_OK;
}

bool LibarchivePlugin::addFiles(QList<Archive::Entry*> &files, const Archive::Entry *destination, const QString &tempDirPath, const CompressionOptions& options)
{
    Q_UNUSED(files)
    Q_UNUSED(options)
    return false;
}

bool LibarchivePlugin::deleteFiles(const QList<Archive::Entry*> &files)
{
    Q_UNUSED(files)
    return false;
}

bool LibarchivePlugin::addComment(const QString& comment)
{
    Q_UNUSED(comment)
    return false;
}

bool LibarchivePlugin::testArchive()
{
    return false;
}

bool LibarchivePlugin::doKill()
{
    m_abortOperation = true;
    return true;
}

bool LibarchivePlugin::copyFiles(const QList<Archive::Entry*>& files, const QString& destinationDirectory, const ExtractionOptions& options)
{
    qCDebug(ARK) << "Changing current directory to " << destinationDirectory;
    QDir::setCurrent(destinationDirectory);

    const bool extractAll = files.isEmpty();
    const bool preservePaths = options.value(QStringLiteral( "PreservePaths" )).toBool();
    bool removeRootNode = options.value(QStringLiteral("RemoveRootNode"), QVariant()).toBool();

    // To avoid traversing the entire archive when extracting a limited set of
    // entries, we maintain a list of remaining entries and stop when it's
    // empty.
    QStringList fullPaths = entryFullPaths(files);
    QStringList remainingFiles = entryFullPaths(files);

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
        // This error might be shown outside of a running Ark part (e.g. from a batch job).
        emit error(xi18nc("@info", "Could not open the archive <filename>%1</filename>.<nl/>"
                                   "Check whether you have sufficient permissions.",
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
    while (!m_abortOperation && (archive_read_next_header(arch.data(), &entry) == ARCHIVE_OK)) {

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
                    //qCDebug(ARK) << "Removing" << files.at(index).value<fileRootNodePair>().rootNode << "from" << entryName;

                    const QString truncatedFilename(entryName.remove(0, rootNode.size()));
                    archive_entry_copy_pathname(entry, QFile::encodeName(truncatedFilename).constData());
                    entryFI = QFileInfo(truncatedFilename);
                }
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

            remainingFiles.removeOne(entryName);

        } else {

            // Archive entry not among selected files, skip it.
            archive_read_data_skip(arch.data());

        }

    } // While entries left to read in archive.

    m_abortOperation = false;

    qCDebug(ARK) << "Extracted" << no_entries << "entries";

    return archive_read_close(arch.data()) == ARCHIVE_OK;
}

void LibarchivePlugin::emitEntryFromArchiveEntry(struct archive_entry *aentry)
{
    Archive::Entry *e = new Archive::Entry(Q_NULLPTR);

#ifdef _MSC_VER
    e->setProperty("fullPath", QDir::fromNativeSeparators(QString::fromUtf16((ushort*)archive_entry_pathname_w(aentry))));
#else
    e->setProperty("fullPath", QDir::fromNativeSeparators(QString::fromWCharArray(archive_entry_pathname_w(aentry))));
#endif

    const QString owner = QString::fromLatin1(archive_entry_uname(aentry));
    if (!owner.isEmpty()) {
        e->setProperty("owner", owner);
    }

    const QString group = QString::fromLatin1(archive_entry_gname(aentry));
    if (!group.isEmpty()) {
        e->setProperty("group", group);
    }

    e->compressedSizeIsSet = false;
    e->setProperty("size", (qlonglong)archive_entry_size(aentry));
    e->setProperty("isDirectory", S_ISDIR(archive_entry_mode(aentry)));

    if (archive_entry_symlink(aentry)) {
        e->setProperty("link", QLatin1String( archive_entry_symlink(aentry) ));
    }

    e->setProperty("timestamp", QDateTime::fromTime_t(archive_entry_mtime(aentry)));

    emit entry(e);
}

int LibarchivePlugin::extractionFlags() const
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

void LibarchivePlugin::copyData(const QString& filename, struct archive *dest, bool partialprogress)
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

void LibarchivePlugin::copyData(const QString& filename, struct archive *source, struct archive *dest, bool partialprogress)
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

#include "libarchiveplugin.moc"
