/*
 *
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#include "app/logging.h"
#include "karchiveplugin.h"
#include "kerfuffle/queries.h"

#include <KZip>
#include <KTar>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KIO/Global>

#include <QDebug>
#include <QFileInfo>
#include <QSet>
#include <QDir>
#include <QMimeDatabase>

Q_LOGGING_CATEGORY(KERFUFFLE_PLUGIN, "ark.kerfuffle.karchive", QtWarningMsg)

K_PLUGIN_FACTORY( KArchiveInterfaceFactory, registerPlugin< KArchiveInterface >(); )

KArchiveInterface::KArchiveInterface(QObject *parent, const QVariantList &args)
        : ReadWriteArchiveInterface(parent, args), m_archive(Q_NULLPTR)
{
    qCDebug(KERFUFFLE_PLUGIN) << "Loaded karchive plugin";
}

KArchiveInterface::~KArchiveInterface()
{
    delete m_archive;
    m_archive = Q_NULLPTR;
}

KArchive *KArchiveInterface::archive()
{
    if (!m_archive) {
        qCDebug(KERFUFFLE_PLUGIN) << "Creating new KArchive instance";

        QMimeDatabase db;
        QMimeType mimeType = db.mimeTypeForFile(filename());

        qCDebug(KERFUFFLE_PLUGIN) << "Archive MIME type is" << mimeType.name() << "inherits ZIP:" << mimeType.inherits(QStringLiteral("application/zip"));

        if (mimeType.inherits(QStringLiteral("application/zip"))) {
            qCDebug(KERFUFFLE_PLUGIN) << "Instance is KZip";
            m_archive = new KZip(filename());
        } else {
            qCDebug(KERFUFFLE_PLUGIN) << "Instance is KTar";
            m_archive = new KTar(filename());
        }

    }
    return m_archive;
}

bool KArchiveInterface::list()
{
    qCDebug(KERFUFFLE_PLUGIN) << "Listing archive contents";

    if (!QFile(archive()->fileName()).exists())
    {
        // If archive doesn't exist, create an empty one
        // Otherwise, subsequent file additions will fail
        qCDebug(KERFUFFLE_PLUGIN) << "Archive doesn't exist, creating an empty one.";
        if (!createEmptyArchive(archive())) {
            return false;
        }
    }

    if (!archive()->isOpen() && !archive()->open(QIODevice::ReadOnly)) {
        qCDebug(KERFUFFLE_PLUGIN) << "Failed to open archive for reading.";
        emit error(xi18nc("@info", "Could not open the archive <filename>%1</filename> for reading.", filename()));
        return false;
    } else {
        qCDebug(KERFUFFLE_PLUGIN) << "Browsing archive" << archive()->fileName();
        m_no_files = 0;
        m_no_dirs = 0;
        bool ret = processDir(archive()->directory());
        qCDebug(KERFUFFLE_PLUGIN) << "Created entries for" << m_no_files << "files" << m_no_dirs << "dirs";
        return ret;
    }
}

bool KArchiveInterface::copyFiles(const QList<QVariant> &files, const QString &destinationDirectory, ExtractionOptions options)
{
    const bool extractAll = files.isEmpty();
    const bool preservePaths = options.value(QLatin1String("PreservePaths")).toBool();

    if (!extractAll) {
        qCDebug(KERFUFFLE_PLUGIN) << "Going to copy" << files.size() << "files.";
    } else {
        qCDebug(KERFUFFLE_PLUGIN) << "Going to copy all files.";
    }

    QString rootNode = options.value(QLatin1String("RootNode"), QVariant()).toString();
    if ((!rootNode.isEmpty()) && (!rootNode.endsWith(QLatin1Char('/')))) {
        rootNode.append(QLatin1Char('/'));
    }

    // Check if archive is readable.
    if (!archive()->isOpen() && !archive()->open(QIODevice::ReadOnly)) {
        emit error(xi18nc("@info", "Could not open the archive <filename>%1</filename> for reading.", filename()));
        return false;
    }

    const KArchiveDirectory *dir = archive()->directory();
    QList<QVariant> extrFiles = files;

    // If extracting all files, populate extrFiles with all entries.
    if (extractAll) {
        getAllEntries(dir, QString(), extrFiles);
    }
    std::sort(extrFiles.begin(), extrFiles.end());

    bool overwriteAll = false;
    bool skipAll = false;
    int no_entries = 0;

    foreach(const QVariant &file, extrFiles) {

        const KArchiveEntry *archiveEntry = dir->entry(file.toString());
        const bool entryIsDir = archiveEntry->isDirectory();

        QFileInfo entryFI(file.toString());
        //qCDebug(KERFUFFLE_PLUGIN) << "entry: " << entryFI.filePath();

        // We skip directories if not preserving paths.
        if (!preservePaths && entryIsDir) {
            continue;
        }

        // Check if the entry exists in the archive.
        if (!archiveEntry) {
            qCDebug(KERFUFFLE_PLUGIN) << "File not found in the archive" << entryFI.fileName();
            emit error(xi18nc("@info", "File <filename>%1</filename> not found in the archive.", entryFI.fileName()));
            return false;
        }

        // If we DON'T preserve paths, we remove the path from entryFI.
        if (!preservePaths) {

            // Empty filenames (ie dirs) should have been skipped already,
            // so asserting.
            Q_ASSERT(!entryFI.fileName().isEmpty());

            entryFI = QFileInfo(entryFI.fileName());

        } else {

            // If rootNode is provided, then we remove the
            // rootNode from the filepath.
            if (!rootNode.isEmpty()) {
                qCDebug(KERFUFFLE_PLUGIN) << "Removing" << rootNode << "from" << entryFI.filePath();

                entryFI.setFile(entryFI.filePath().remove(0, rootNode.size()));
                qCDebug(KERFUFFLE_PLUGIN) << "entry: " << entryFI.filePath();
            }
        }

        // The final destination with full path.
        QString finalDest(destinationDirectory + QLatin1Char('/') + entryFI.filePath());

        // The parent of directory of final destination with full path.
        QString finalDestDir(destinationDirectory + QLatin1Char('/') + entryFI.path());

        // Handle files.
        if (!entryIsDir) {

            if (QFile::exists(finalDest) && !overwriteAll) {

                // File exists, query user for action.

                qCWarning(KERFUFFLE_PLUGIN) << "File exists:" << finalDest;

                // User previously chose to skip all existing files.
                if (skipAll) {
                    continue;
                }

                Kerfuffle::OverwriteQuery query(finalDest);
                query.setNoRenameMode(true);
                emit userQuery(&query);
                query.waitForResponse();

                if (query.responseSkip() || query.responseAutoSkip()) {
                    if (query.responseAutoSkip()) {
                        skipAll = true;
                    }
                    continue;
                } else if (query.responseCancelled()) {
                    break;
                } else if (query.responseOverwriteAll()) {
                    overwriteAll = true;
                }

            }

            // Copy the file.

            //qCDebug(KERFUFFLE_PLUGIN) << "Writing file:" << finalDest;

            // If parent directory doesn't exist, create it.
            if (!QFileInfo(finalDestDir).exists()) {
                qCDebug(KERFUFFLE_PLUGIN) << "Creating parent directory for file:" << finalDest;
                if (!QDir().mkpath(finalDestDir)) {
                    qCCritical(KERFUFFLE_PLUGIN) << "Failed to create parent directory for file:" << entryFI.filePath();
                    continue;
                }
            } else {

            }

            if (!QFileInfo(finalDestDir).isWritable()) {
                qCCritical(KERFUFFLE_PLUGIN) << "Parent directory is not writable for file:" << entryFI.filePath();
                continue;
            }

            if (archiveEntry->symLinkTarget().isEmpty()) {
                static_cast<const KArchiveFile*>(archiveEntry)->copyTo(finalDestDir);
            } else {
                // Create symlink
                QFile::link(archiveEntry->symLinkTarget(), QString(finalDest));
            }

        } else {

            // Handle directories. This is needed, because KArchiveFile::copyTo()
            // doesn't create missing parent directories. Also needed for extraction
            // of empty directories.

            if (!QFileInfo(finalDest).exists()) {
                //qCDebug(KERFUFFLE_PLUGIN) << "Creating directory: " << finalDest;
                if (!QDir().mkpath(finalDest)) {
                    qCCritical(KERFUFFLE_PLUGIN) << "Failed to create directory" << entryFI.filePath();
                    continue;
                }
            }
        }
        no_entries++;

    }
    qCDebug(KERFUFFLE_PLUGIN) << "Extracted" << no_entries << "entries";

    return true;
}

void KArchiveInterface::getAllEntries(const KArchiveDirectory *dir, const QString &prefix, QList<QVariant> &list)
{
    // Traverse dir recursively and append all files and subdirectories to list.

    foreach(const QString &entryName, dir->entries()) {
        const KArchiveEntry *entry = dir->entry(entryName);
        if (entry->isDirectory()) {
            QString newPrefix = (prefix.isEmpty() ? prefix : prefix + QLatin1Char('/')) + entryName;
            getAllEntries(static_cast<const KArchiveDirectory*>(entry), newPrefix, list);
        }
        if (prefix.isEmpty()) {
            list.append(entryName);
        } else {
            list.append(prefix + QLatin1Char('/') + entryName);
        }
    }
}

int KArchiveInterface::handleFileExistsMessage(const QString &dir, const QString &fileName)
{
    Kerfuffle::OverwriteQuery query(dir + QLatin1Char('/') + fileName);
    query.setNoRenameMode(true);
    emit userQuery(&query);
    query.waitForResponse();

    if (query.responseOverwrite()) {
        return OverwriteYes;
    } else if (query.responseSkip()) {
        return OverwriteSkip;
    } else if (query.responseOverwriteAll()) {
        return OverwriteAll;
    } else if (query.responseAutoSkip()) {
        return OverwriteAutoSkip;
    }

    return OverwriteCancel;
}

bool KArchiveInterface::processDir(const KArchiveDirectory *dir, const QString & prefix)
{
    // Processes a directory recursively and creates KArchiveEntry's.

    // qCDebug(KERFUFFLE_PLUGIN) << "Processing directory" << dir->name();

    foreach(const QString& entryName, dir->entries()) {
        const KArchiveEntry *entry = dir->entry(entryName);
        createEntryFor(entry, prefix);
        if (entry->isDirectory()) {
            QString newPrefix = (prefix.isEmpty() ? prefix : prefix + QLatin1Char('/')) + entryName;
            processDir(static_cast<const KArchiveDirectory*>(entry), newPrefix);
        }
    }
    m_no_dirs++;
    return true;
}

void KArchiveInterface::createEntryFor(const KArchiveEntry *aentry, const QString& prefix)
{
    //qCDebug(KERFUFFLE_PLUGIN) << "Creating archive entry";

    ArchiveEntry e;
    QString fileName = prefix.isEmpty() ? aentry->name() : prefix + QLatin1Char('/') + aentry->name();

    if (aentry->isDirectory() && !fileName.endsWith(QLatin1Char('/'))) {
        fileName += QLatin1Char('/');
    } else {
      m_no_files++;
    }

    e[ FileName ]         = fileName;
    e[ InternalID ]       = e[ FileName ];
    e[ Permissions ]      = permissionsString(aentry->permissions());
    e[ Owner ]            = aentry->user();
    e[ Group ]            = aentry->group();
    e[ IsDirectory ]      = aentry->isDirectory();
    e[ Timestamp ]        = aentry->date();
    if (!aentry->symLinkTarget().isEmpty()) {
        e[ Link ]             = aentry->symLinkTarget();
    }
    if (aentry->isFile()) {
        e[ Size ] = static_cast<const KArchiveFile*>(aentry)->size();
    }
    else {
        e[ Size ] = 0;
    }
    emit entry(e);


}

bool KArchiveInterface::addFiles(const QStringList &files, const Kerfuffle::CompressionOptions &options)
{
    Q_UNUSED(options)
    qCDebug(KERFUFFLE_PLUGIN) << "Going to add files to" << archive()->fileName();

    // Close archive if open
    if (archive()->isOpen()) {
        qCDebug(KERFUFFLE_PLUGIN) << "Archive was open, closing...";
        archive()->close();
    }

    // We need to delete KArchive instance, otherwise we risk KArchive::device()
    // not returning a NULL pointer, which KArchive::open() doesn't tolerate.
    delete m_archive;
    m_archive = Q_NULLPTR;

    Q_ASSERT(!archive()->device());

    // Open the archive
    if (!archive()->open(QIODevice::ReadWrite)) {
        emit error(xi18nc("@info", "Could not open the archive <filename>%1</filename> for writing.", filename()));
        return false;
    }

    qCDebug(KERFUFFLE_PLUGIN) << "Archive opened for writing...";
    qCDebug(KERFUFFLE_PLUGIN) << "Will add " << files.count() << " files";
    foreach(const QString &path, files) {
        qCDebug(KERFUFFLE_PLUGIN) << "Adding " << path;
        QFileInfo fi(path);
        Q_ASSERT(fi.exists());

        if (fi.isDir()) {
            if (archive()->addLocalDirectory(path, fi.fileName())) {
                const KArchiveEntry *entry = archive()->directory()->entry(fi.fileName());
                createEntryFor(entry, QString());
                processDir((KArchiveDirectory*) archive()->directory()->entry(fi.fileName()), fi.fileName());
            } else {
                emit error(xi18nc("@info", "Could not add the directory <filename>%1</filename> to the archive", path));
                return false;
            }
        } else {
            if (archive()->addLocalFile(path, fi.fileName())) {
                qCDebug(KERFUFFLE_PLUGIN) << "Successfully added file" << fi.fileName();

                // The data is written to a QTemporaryFile when using addLocalFile().
                // We need to call close() to write to the actual archive, otherwise
                // fetching the KArchiveEntry fails.
                archive()->close();
                if (!archive()->open(QIODevice::ReadWrite)) {
                    emit error(xi18nc("@info", "Could not open the archive <filename>%1</filename> after adding file.", filename()));
                    return false;
                }

                qCDebug(KERFUFFLE_PLUGIN) << "Archive now contains" << archive()->directory()->entries();

                const KArchiveEntry *entry = archive()->directory()->entry(fi.fileName());
                createEntryFor(entry, QString());
            } else {
                emit error(xi18nc("@info", "Could not add the file <filename>%1</filename> to the archive.", path));
                return false;
            }
        }
    }
    qCDebug(KERFUFFLE_PLUGIN) << "Closing the archive";
    archive()->close();
    qCDebug(KERFUFFLE_PLUGIN) << "Done";
    return true;
}

bool KArchiveInterface::deleteFiles(const QList<QVariant> & files)
{
    Q_UNUSED(files)

    qCDebug(KERFUFFLE_PLUGIN) << "Going to delete files" << files;

    return false;
}

// Borrowed and adapted from KFileItemPrivate::parsePermissions.
QString KArchiveInterface::permissionsString(mode_t perm)
{
    static char buffer[ 12 ];

    char uxbit,gxbit,oxbit;

    if ( (perm & (S_IXUSR|S_ISUID)) == (S_IXUSR|S_ISUID) ) {
        uxbit = 's';
    } else if ( (perm & (S_IXUSR|S_ISUID)) == S_ISUID ) {
        uxbit = 'S';
    } else if ( (perm & (S_IXUSR|S_ISUID)) == S_IXUSR ) {
        uxbit = 'x';
    } else {
        uxbit = '-';
    }

    if ( (perm & (S_IXGRP|S_ISGID)) == (S_IXGRP|S_ISGID) ) {
        gxbit = 's';
    } else if ( (perm & (S_IXGRP|S_ISGID)) == S_ISGID ) {
        gxbit = 'S';
    } else if ( (perm & (S_IXGRP|S_ISGID)) == S_IXGRP ) {
        gxbit = 'x';
    } else {
        gxbit = '-';
    }

    if ( (perm & (S_IXOTH|S_ISVTX)) == (S_IXOTH|S_ISVTX) ) {
        oxbit = 't';
    } else if ( (perm & (S_IXOTH|S_ISVTX)) == S_ISVTX ) {
        oxbit = 'T';
    } else if ( (perm & (S_IXOTH|S_ISVTX)) == S_IXOTH ) {
        oxbit = 'x';
    } else {
        oxbit = '-';
    }

    // Include the type in the first char like kde3 did; people are more used to seeing it,
    // even though it's not really part of the permissions per se.
    if (S_ISDIR(perm)) {
        buffer[0] = 'd';
    } else if (S_ISLNK(perm)) {
        buffer[0] = 'l';
    } else {
        buffer[0] = '-';
    }

    buffer[1] = ((( perm & S_IRUSR ) == S_IRUSR ) ? 'r' : '-' );
    buffer[2] = ((( perm & S_IWUSR ) == S_IWUSR ) ? 'w' : '-' );
    buffer[3] = uxbit;
    buffer[4] = ((( perm & S_IRGRP ) == S_IRGRP ) ? 'r' : '-' );
    buffer[5] = ((( perm & S_IWGRP ) == S_IWGRP ) ? 'w' : '-' );
    buffer[6] = gxbit;
    buffer[7] = ((( perm & S_IROTH ) == S_IROTH ) ? 'r' : '-' );
    buffer[8] = ((( perm & S_IWOTH ) == S_IWOTH ) ? 'w' : '-' );
    buffer[9] = oxbit;
    buffer[10] = 0;

    return QString::fromLatin1(buffer);
}

bool KArchiveInterface::createEmptyArchive(KArchive *archive)
{
    // Create an empty archive.

    QFile arch(archive->fileName());

    QMimeDatabase db;
    QMimeType mimeType = db.mimeTypeForFile(filename());

    if (mimeType.inherits(QStringLiteral("application/zip"))) {
        qCDebug(KERFUFFLE_PLUGIN) << "Creating empty zip archive:" << archive->fileName();

        // Create an empty zip archive
        FILE* f;
        if ((f = fopen(filename().toLatin1(), "wb")) &&
            (fwrite( "\x50\x4B\x05\x06\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 22, 1, f )))
        {
            fclose(f);
            qCDebug(KERFUFFLE_PLUGIN) << "Successfully created empty zip archive.";
            return true;
        }
    } else {
        qCDebug(KERFUFFLE_PLUGIN) << "Creating empty tar-based archive:" << archive->fileName();
        if (arch.open(QIODevice::ReadWrite))
        {
            arch.close();
            qCDebug(KERFUFFLE_PLUGIN) << "Successfully created empty tar-based archive.";
            return true;
        }
    }
    qCWarning(KERFUFFLE_PLUGIN) << "Failed to create an empty archive!";
    return false;
}

#include "karchiveplugin.moc"
