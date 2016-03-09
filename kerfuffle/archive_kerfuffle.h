/*
 * Copyright (c) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (c) 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (c) 2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
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

#ifndef ARCHIVE_H
#define ARCHIVE_H

#include "kerfuffle_export.h"

#include <QHash>
#include <QMimeType>
#include <QObject>
#include <QVariant>


#include <KPluginMetaData>

class KJob;

namespace Kerfuffle
{
class ListJob;
class ExtractJob;
class DeleteJob;
class AddJob;
class Query;
class ReadOnlyArchiveInterface;

/**
 * Meta data related to one entry in a compressed archive.
 *
 * When creating a plugin, information about every single entry in
 * an archive is contained in an ArchiveEntry, and metadata
 * is set with the entries in this enum.
 *
 * Please notice that not all archive formats support all the properties
 * below, so set those that are available.
 */
enum EntryMetaDataType {
    FileName = 0,        /**< The entry's file name */
    InternalID,          /**< The entry's ID for Ark's internal manipulation */
    Permissions,         /**< The entry's permissions */
    Owner,               /**< The user the entry belongs to */
    Group,               /**< The user group the entry belongs to */
    Size,                /**< The entry's original size */
    CompressedSize,      /**< The compressed size for the entry */
    Link,                /**< The entry is a symbolic link */
    Ratio,               /**< The compression ratio for the entry */
    CRC,                 /**< The entry's CRC */
    Method,              /**< The compression method used on the entry */
    Version,             /**< The archiver version needed to extract the entry */
    Timestamp,           /**< The timestamp for the current entry */
    IsDirectory,         /**< The entry is a directory */
    Comment,
    IsPasswordProtected, /**< The entry is password-protected */
    Custom = 1048576
};

enum ArchiveError {
    NoError = 0,
    NoPlugin,
    FailedPlugin
};

typedef QHash<int, QVariant> ArchiveEntry;

/**
These are the extra options for doing the compression. Naming convention
is CamelCase with either Global, or the compression type (such as Zip,
Rar, etc), followed by the property name used
 */
typedef QHash<QString, QVariant> CompressionOptions;
typedef QHash<QString, QVariant> ExtractionOptions;

/**
 * Stores a filename and rootnode pair. This is used to cut an individual
 * rootnode from the path of each file, e.g. when drag'n'drop extracting a
 * selection of files.
 */
struct fileRootNodePair
{
    QString file;
    QString rootNode;

    fileRootNodePair()
    {}

    fileRootNodePair(const QString &f)
        : file(f)
    {}

    fileRootNodePair(const QString &f, const QString &n)
        : file(f),
          rootNode(n)
    {}

    // Required to compare QVariants with this type.
    bool operator==(const fileRootNodePair &right) const
    {
        if (file == right.file)
            return true;
        else
            return false;
    }
    bool operator<(const fileRootNodePair &) const
    {
        return false;
    }
};
QDebug operator<<(QDebug d, const fileRootNodePair &pair);

class KERFUFFLE_EXPORT Archive : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString fileName READ fileName)
    Q_PROPERTY(QMimeType mimeType READ mimeType)
    Q_PROPERTY(bool isReadOnly READ isReadOnly)
    Q_PROPERTY(bool isPasswordProtected READ isPasswordProtected)
    Q_PROPERTY(bool hasComment READ hasComment)
    Q_PROPERTY(qulonglong numberOfFiles READ numberOfFiles)
    Q_PROPERTY(qulonglong unpackedSize READ unpackedSize)
    Q_PROPERTY(qulonglong packedSize READ packedSize)

public:
    QString fileName() const;
    QMimeType mimeType() const;
    qulonglong numberOfFiles() const;
    bool isReadOnly() const;
    bool isPasswordProtected();
    bool hasComment() const;
    qulonglong unpackedSize() const;
    qulonglong packedSize() const;

    static bool comparePlugins(const KPluginMetaData &p1, const KPluginMetaData &p2);
    static QMimeType determineMimeType(const QString& filename);
    static QVector<KPluginMetaData> findPluginOffers(const QString& filename, const QString& fixedMimeType);

    static Archive *create(const QString &fileName, QObject *parent = 0);
    static Archive *create(const QString &fileName, const QString &fixedMimeType, QObject *parent = 0);
    ~Archive();

    ArchiveError error() const;
    bool isValid() const;


    /**
     * @return QFileInfo(fileName()).completeBaseName() without the "tar" extension (if any).
     */
    QString completeBaseName() const;

    KJob* open();
    KJob* create();
    ListJob* list();
    DeleteJob* deleteFiles(const QList<QVariant> & files);

    /**
     * Compression options that should be handled by all interfaces:
     *
     * GlobalWorkDir - Change to this dir before adding the new files.
     * The path names should then be added relative to this directory.
     *
     * TODO: find a way to actually add files to specific locations in
     * the archive
     * (not supported yet) GlobalPathInArchive - a path relative to the
     * archive root where the files will be added under
     *
     */
    AddJob* addFiles(const QStringList & files, const CompressionOptions& options = CompressionOptions());

    ExtractJob* copyFiles(const QList<QVariant> &files, const QString &destinationDir, const ExtractionOptions &options = ExtractionOptions());

    bool isSingleFolderArchive();
    QString subfolderName();

    void setPassword(const QString &password);
    void enableHeaderEncryption(bool enable);
    QString comment() const;

private slots:
    void onListFinished(KJob*);
    void onAddFinished(KJob*);
    void onUserQuery(Kerfuffle::Query*);
    void onNewEntry(const ArchiveEntry &entry);

private:
    Archive(ReadOnlyArchiveInterface *archiveInterface, bool isReadOnly, QObject *parent = 0);
    Archive(ArchiveError errorCode, QObject *parent = 0);

    void listIfNotListed();
    ReadOnlyArchiveInterface *m_iface;
    bool m_hasBeenListed;
    bool m_isReadOnly;
    bool m_isPasswordProtected;
    bool m_isSingleFolderArchive;

    QString m_subfolderName;
    qulonglong m_extractedFilesSize;
    ArchiveError m_error;
    int m_numberOfFiles;
};

KERFUFFLE_EXPORT QSet<QString> supportedMimeTypes();
KERFUFFLE_EXPORT QSet<QString> supportedWriteMimeTypes();
KERFUFFLE_EXPORT QSet<QString> supportedEncryptEntriesMimeTypes();
KERFUFFLE_EXPORT QSet<QString> supportedEncryptHeaderMimeTypes();
} // namespace Kerfuffle

Q_DECLARE_METATYPE(Kerfuffle::fileRootNodePair)

#endif // ARCHIVE_H
