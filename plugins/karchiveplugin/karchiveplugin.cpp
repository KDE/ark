/*
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
#include "karchiveplugin.h"
#include "kerfuffle/queries.h"

#include <KZip>
#include <KTar>
#include <KMimeType>
#include <KDebug>
#include <KLocale>
#include <QDir>

#include <QFileInfo>
#include <QSet>

KArchiveInterface::KArchiveInterface(QObject *parent, const QVariantList &args)
        : ReadWriteArchiveInterface(parent, args), m_archive(0)
{
    kDebug();
}

KArchiveInterface::~KArchiveInterface()
{
    delete m_archive;
    m_archive = 0;
}

KArchive *KArchiveInterface::archive()
{
    if (m_archive == 0) {
        KMimeType::Ptr mimeType = KMimeType::findByPath(filename());

        if (mimeType->is(QLatin1String("application/zip"))) {
            m_archive = new KZip(filename());
        } else {
            m_archive = new KTar(filename());
        }

    }
    return m_archive;
}

bool KArchiveInterface::list()
{
    kDebug();
    if (!archive()->isOpen() && !archive()->open(QIODevice::ReadOnly)) {
        emit error(i18nc("@info", "Could not open the archive <filename>%1</filename> for reading", filename()));
        return false;
    } else {
        return browseArchive(archive());
    }
}

void KArchiveInterface::getAllEntries(const KArchiveDirectory *dir, const QString &prefix, QList< QVariant > &list)
{
    foreach(const QString &entryName, dir->entries()) {
        const KArchiveEntry *entry = dir->entry(entryName);
        if (entry->isDirectory()) {
            QString newPrefix = (prefix.isEmpty() ? prefix : prefix + QLatin1Char('/')) + entryName;
            getAllEntries(static_cast<const KArchiveDirectory*>(entry), newPrefix, list);
        }
        else {
            list.append(prefix + QLatin1Char('/') + entryName);
        }
    }
}

bool KArchiveInterface::copyFiles(const QList<QVariant> &files, const QString &destinationDirectory, ExtractionOptions options)
{
    const bool preservePaths = options.value(QLatin1String("PreservePaths")).toBool();
    const KArchiveDirectory *dir = archive()->directory();

    if (!archive()->isOpen() && !archive()->open(QIODevice::ReadOnly)) {
        emit error(i18nc("@info", "Could not open the archive <filename>%1</filename> for reading", filename()));
        return false;
    }

    QList<QVariant> extrFiles = files;
    if (extrFiles.isEmpty()) { // All files should be extracted
        getAllEntries(dir, QString(), extrFiles);
    }

    bool overwriteAllSelected = false;
    bool autoSkipSelected = false;
    QSet<QString> dirCache;
    foreach(const QVariant &file, extrFiles) {
        QString realDestination = destinationDirectory;
        const KArchiveEntry *archiveEntry = dir->entry(file.toString());
        if (!archiveEntry) {
            emit error(i18nc("@info", "File <filename>%1</filename> not found in the archive" , file.toString()));
            return false;
        }

        if (preservePaths) {
            QFileInfo fi(file.toString());
            QDir dest(destinationDirectory);
            QString filepath = archiveEntry->isDirectory() ? fi.filePath() : fi.path();
            if (!dirCache.contains(filepath)) {
                if (!dest.mkpath(filepath)) {
                    emit error(i18nc("@info", "Error creating directory <filename>%1</filename>", filepath));
                    return false;
                }
                dirCache << filepath;
            }
            realDestination = dest.absolutePath() + QLatin1Char('/') + filepath;
        }

        // TODO: handle errors, copyTo fails silently
        if (!archiveEntry->isDirectory()) { // We don't need to do anything about directories
            if (QFile::exists(realDestination + QLatin1Char('/') + archiveEntry->name()) && !overwriteAllSelected) {
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
            }
            else {
                static_cast<const KArchiveFile*>(archiveEntry)->copyTo(realDestination);
            }
        }
    }

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
    return processDir(archive->directory());
}

bool KArchiveInterface::processDir(const KArchiveDirectory *dir, const QString & prefix)
{
    foreach(const QString& entryName, dir->entries()) {
        const KArchiveEntry *entry = dir->entry(entryName);
        createEntryFor(entry, prefix);
        if (entry->isDirectory()) {
            QString newPrefix = (prefix.isEmpty() ? prefix : prefix + QLatin1Char('/')) + entryName;
            processDir(static_cast<const KArchiveDirectory*>(entry), newPrefix);
        }
    }
    return true;
}

void KArchiveInterface::createEntryFor(const KArchiveEntry *aentry, const QString& prefix)
{
    ArchiveEntry e;
    QString fileName = prefix.isEmpty() ? aentry->name() : prefix + QLatin1Char('/') + aentry->name();

    if (aentry->isDirectory() && !fileName.endsWith(QLatin1Char('/')))
        fileName += QLatin1Char('/');

    e[ FileName ]         = fileName;
    e[ InternalID ]       = e[ FileName ];
    e[ Permissions ]      = permissionsString(aentry->permissions());
    e[ Owner ]            = aentry->user();
    e[ Group ]            = aentry->group();
    e[ IsDirectory ]      = aentry->isDirectory();
    e[ Timestamp ]        = aentry->datetime();
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
    kDebug() << "Starting...";
//  delete m_archive;
//  m_archive = 0;
    if (archive()->isOpen()) {
        archive()->close();
    }
    if (!archive()->open(QIODevice::ReadWrite)) {
        emit error(i18nc("@info", "Could not open the archive <filename>%1</filename> for writing.", filename()));
        return false;
    }

    kDebug() << "Archive opened for writing...";
    kDebug() << "Will add " << files.count() << " files";
    foreach(const QString &path, files) {
        kDebug() << "Adding " << path;
        QFileInfo fi(path);
        Q_ASSERT(fi.exists());

        if (fi.isDir()) {
            if (archive()->addLocalDirectory(path, fi.fileName())) {
                const KArchiveEntry *entry = archive()->directory()->entry(fi.fileName());
                createEntryFor(entry, QString());
                processDir((KArchiveDirectory*) archive()->directory()->entry(fi.fileName()), fi.fileName());
            } else {
                emit error(i18nc("@info", "Could not add the directory <filename>%1</filename> to the archive", path));
                return false;
            }
        } else {
            if (archive()->addLocalFile(path, fi.fileName())) {
                const KArchiveEntry *entry = archive()->directory()->entry(fi.fileName());
                createEntryFor(entry, QString());
            } else {
                emit error(i18nc("@info", "Could not add the file <filename>%1</filename> to the archive.", path));
                return false;
            }
        }
    }
    kDebug() << "Closing the archive";
    archive()->close();
    kDebug() << "Done";
    return true;
}

bool KArchiveInterface::deleteFiles(const QList<QVariant> & files)
{
    Q_UNUSED(files)
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

KERFUFFLE_EXPORT_PLUGIN(KArchiveInterface)
