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
#include "ark_debug.h"
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

K_PLUGIN_FACTORY_WITH_JSON(KArchiveInterfaceFactory, "kerfuffle_karchive.json", registerPlugin<KArchiveInterface>();)

KArchiveInterface::KArchiveInterface(QObject *parent, const QVariantList &args)
        : ReadWriteArchiveInterface(parent, args), m_archive(Q_NULLPTR)
{
    qCDebug(ARK) << "Loaded karchive plugin";
}

KArchiveInterface::~KArchiveInterface()
{
    delete m_archive;
    m_archive = Q_NULLPTR;
}

KArchive *KArchiveInterface::archive()
{
    if (!m_archive) {
        qCDebug(ARK) << "Creating new KArchive instance";

        QMimeDatabase db;
        QMimeType mimeType = db.mimeTypeForFile(filename());

        qCDebug(ARK) << "Archive MIME type is" << mimeType.name() << "inherits ZIP:" << mimeType.inherits(QStringLiteral("application/zip"));

        if (mimeType.inherits(QStringLiteral("application/zip"))) {
            qCDebug(ARK) << "Instance is KZip";
            m_archive = new KZip(filename());
        } else {
            qCDebug(ARK) << "Instance is KTar";
            m_archive = new KTar(filename());
        }

    }
    return m_archive;
}

bool KArchiveInterface::list()
{
    qCDebug(ARK) << "Listing archive contents";

    if (!QFile(archive()->fileName()).exists())
    {
        // If archive doesn't exist, create an empty one
        // Otherwise, subsequent file additions will fail
        qCDebug(ARK) << "Archive doesn't exist, creating an empty one.";
        if (!createEmptyArchive(archive())) {
            return false;
        }
    }

    if (!archive()->isOpen() && !archive()->open(QIODevice::ReadOnly)) {
        qCDebug(ARK) << "Failed to open archive for reading.";
        emit error(xi18nc("@info", "Could not open the archive <filename>%1</filename> for reading", filename()));

        return false;
    } else {
        return browseArchive(archive());
    }
}

void KArchiveInterface::getAllEntries(const KArchiveDirectory *dir, const QString &prefix, QList< QVariant > &list)
{
    // Traverse the archive root directory recursively
    // and append all files and empty directories to list

    foreach(const QString &entryName, dir->entries()) {
        const KArchiveEntry *entry = dir->entry(entryName);
        if (entry->isDirectory()) {
            QString newPrefix = (prefix.isEmpty() ? prefix : prefix + QLatin1Char('/')) + entryName;
            getAllEntries(static_cast<const KArchiveDirectory*>(entry), newPrefix, list);

            // Find empty directories
            if (static_cast<const KArchiveDirectory*>(entry)->entries().isEmpty())
            {
                qCDebug(ARK) << "Found empty directory" << entry->name();
                if (prefix.isEmpty())
                    list.append(entryName);
                else
                    list.append(prefix + QLatin1Char('/') + entryName);
            }
        }
        else {
            list.append(prefix + QLatin1Char('/') + entryName);
        }
    }
}

bool KArchiveInterface::copyFiles(const QList<QVariant> &files, const QString &destinationDirectory, ExtractionOptions options)
{
    if (files.size() != 0)
        qCDebug(ARK) << "Going to copy" << files.size() << "files";
    else
        qCDebug(ARK) << "Going to copy all files";

    const bool preservePaths = options.value(QLatin1String("PreservePaths")).toBool();
    const KArchiveDirectory *dir = archive()->directory();

    if (!archive()->isOpen() && !archive()->open(QIODevice::ReadOnly)) {
        emit error(xi18nc("@info", "Could not open the archive <filename>%1</filename> for reading", filename()));
        return false;
    }

    QList<QVariant> extrFiles = files;
    if (extrFiles.isEmpty()) { // All files should be extracted
        getAllEntries(dir, QString(), extrFiles);
    }

    bool overwriteAllSelected = false;
    bool autoSkipSelected = false;
    QSet<QString> dirCache;

    int no_files = 0;
    foreach(const QVariant &file, extrFiles) {
        QString realDestination = destinationDirectory;
        qCDebug(ARK) << "Real destination" << realDestination;
        const KArchiveEntry *archiveEntry = dir->entry(file.toString());
        if (!archiveEntry) {
            qCDebug(ARK) << "File not found in the archive" << file.toString();
            emit error(xi18nc("@info", "File <filename>%1</filename> not found in the archive" , file.toString()));
            return false;
        }

        if (preservePaths) {
            QFileInfo fi(file.toString());
            QDir dest(destinationDirectory);
            QString filepath = archiveEntry->isDirectory() ? fi.filePath() : fi.path();
            if (!dirCache.contains(filepath)) {
                if (!dest.mkpath(filepath)) {
                    emit error(xi18nc("@info", "Error creating directory <filename>%1</filename>", filepath));
                    return false;
                }
                dirCache << filepath;
            }
            realDestination = dest.absolutePath() + QLatin1Char('/') + filepath;
            qCDebug(ARK) << "Real destination 2" << realDestination;
        }

        // TODO: handle errors, copyTo fails silently
        if (!archiveEntry->isDirectory()) { // We don't need to do anything about directories

            qCDebug(ARK) << "Extracting file" << archiveEntry->name();

            if (QFile::exists(realDestination + QLatin1Char('/') + archiveEntry->name()) && !overwriteAllSelected) {

                qCDebug(ARK) << "file exists" << realDestination + QLatin1Char('/') + archiveEntry->name();

                if (autoSkipSelected) {
                    continue;
                }

                int response = handleFileExistsMessage(realDestination, archiveEntry->name());

                if (response == OverwriteCancel) {
                    break;
                }
                if (response == OverwriteYes || response == OverwriteAll) {
                    static_cast<const KArchiveFile*>(archiveEntry)->copyTo(realDestination);
                    if (response == OverwriteAll) {
                        overwriteAllSelected = true;
                    }
                }
                if (response == OverwriteAutoSkip) {
                    autoSkipSelected = true;
                }
            } else {

                qCDebug(ARK) << "Extracting file" << realDestination << archiveEntry->name();

                if (archiveEntry->symLinkTarget().isEmpty()) {
                    static_cast<const KArchiveFile*>(archiveEntry)->copyTo(realDestination);
                    QFile::setPermissions(realDestination + QStringLiteral("/") + archiveEntry->name(), KIO::convertPermissions(archiveEntry->permissions()));
                }
                else {
                    // Create symlink
                    QFile::link(archiveEntry->symLinkTarget(), QString(realDestination + QStringLiteral("/") + archiveEntry->name()));
                }
            }
        }
        no_files++;


    }
    qCDebug(ARK) << "Extracted" << no_files << "files";

    return true;
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

bool KArchiveInterface::browseArchive(KArchive *archive)
{
    qCDebug(ARK) << "Browsing archive" << archive->fileName();
    m_no_files = 0;
    m_no_dirs = 0;
    bool ret = processDir(archive->directory());
    qCDebug(ARK) << "Created entries for" << m_no_files << "files" << m_no_dirs << "dirs";
    return ret;
}

bool KArchiveInterface::processDir(const KArchiveDirectory *dir, const QString & prefix)
{
    qCDebug(ARK) << "Processing directory" << dir->name();

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
    qCDebug(ARK) << "Creating archive entry";

    ArchiveEntry e;
    QString fileName = prefix.isEmpty() ? aentry->name() : prefix + QLatin1Char('/') + aentry->name();

    if (aentry->isDirectory() && !fileName.endsWith(QLatin1Char('/')))
        fileName += QLatin1Char('/');
    else
      m_no_files++;

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
    qCDebug(ARK) << "Going to add files to" << archive()->fileName();

    // Close archive if open
    if (archive()->isOpen()) {
        qCDebug(ARK) << "Archive was open, closing...";
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

    qCDebug(ARK) << "Archive opened for writing...";
    qCDebug(ARK) << "Will add " << files.count() << " files";
    foreach(const QString &path, files) {
        qCDebug(ARK) << "Adding " << path;
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
                qCDebug(ARK) << "Successfully added file" << fi.fileName();

                // The data is written to a QTemporaryFile when using addLocalFile().
                // We need to call close() to write to the actual archive, otherwise
                // fetching the KArchiveEntry fails.
                archive()->close();
                if (!archive()->open(QIODevice::ReadWrite)) {
                    emit error(xi18nc("@info", "Could not open the archive <filename>%1</filename> after adding file.", filename()));
                    return false;
                }

                qCDebug(ARK) << "Archive now contains" << archive()->directory()->entries();

                const KArchiveEntry *entry = archive()->directory()->entry(fi.fileName());
                createEntryFor(entry, QString());
            } else {
                emit error(xi18nc("@info", "Could not add the file <filename>%1</filename> to the archive.", path));
                return false;
            }
        }
    }
    qCDebug(ARK) << "Closing the archive";
    archive()->close();
    qCDebug(ARK) << "Done";
    return true;
}

bool KArchiveInterface::deleteFiles(const QList<QVariant> & files)
{
    Q_UNUSED(files)

    qCDebug(ARK) << "Going to delete files" << files;

    return false;
}

// Borrowed and adapted from KFileItemPrivate::parsePermissions.
QString KArchiveInterface::permissionsString(mode_t perm)
{
    static char buffer[ 12 ];

    char uxbit,gxbit,oxbit;

    if ( (perm & (S_IXUSR|S_ISUID)) == (S_IXUSR|S_ISUID) )
        uxbit = 's';
    else if ( (perm & (S_IXUSR|S_ISUID)) == S_ISUID )
        uxbit = 'S';
    else if ( (perm & (S_IXUSR|S_ISUID)) == S_IXUSR )
        uxbit = 'x';
    else
        uxbit = '-';

    if ( (perm & (S_IXGRP|S_ISGID)) == (S_IXGRP|S_ISGID) )
        gxbit = 's';
    else if ( (perm & (S_IXGRP|S_ISGID)) == S_ISGID )
        gxbit = 'S';
    else if ( (perm & (S_IXGRP|S_ISGID)) == S_IXGRP )
        gxbit = 'x';
    else
        gxbit = '-';

    if ( (perm & (S_IXOTH|S_ISVTX)) == (S_IXOTH|S_ISVTX) )
        oxbit = 't';
    else if ( (perm & (S_IXOTH|S_ISVTX)) == S_ISVTX )
        oxbit = 'T';
    else if ( (perm & (S_IXOTH|S_ISVTX)) == S_IXOTH )
        oxbit = 'x';
    else
        oxbit = '-';

    // Include the type in the first char like kde3 did; people are more used to seeing it,
    // even though it's not really part of the permissions per se.
    if (S_ISDIR(perm))
        buffer[0] = 'd';
    else if (S_ISLNK(perm))
        buffer[0] = 'l';
    else
        buffer[0] = '-';

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
        qCDebug(ARK) << "Creating empty zip archive:" << archive->fileName();

        // Create an empty zip archive
        FILE* f;
        if ((f = fopen(filename().toLatin1(), "wb")) &&
            (fwrite( "\x50\x4B\x05\x06\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 22, 1, f )))
        {
            fclose(f);
            qCDebug(ARK) << "Successfully created empty zip archive.";
            return true;
        }
    } else {
        qCDebug(ARK) << "Creating empty tar-based archive:" << archive->fileName();
        if (arch.open(QIODevice::ReadWrite))
        {
            arch.close();
            qCDebug(ARK) << "Successfully created empty tar-based archive.";
            return true;
        }
    }
    qCWarning(ARK) << "Failed to create an empty archive!";
    return false;
}

#include "karchiveplugin.moc"
