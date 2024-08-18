/*
    SPDX-FileCopyrightText: 2007 Henrique Pinto <henrique.pinto@kdemail.net>
    SPDX-FileCopyrightText: 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
    SPDX-FileCopyrightText: 2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef ARCHIVE_H
#define ARCHIVE_H

#include "kerfuffle_export.h"
#include "metadatabackup.h"
#include "options.h"

#include <KJob>

#include <QHash>
#include <QMimeType>

namespace Kerfuffle
{
class LoadJob;
class BatchExtractJob;
class CreateJob;
class ExtractJob;
class DeleteJob;
class AddJob;
class MoveJob;
class CopyJob;
class CommentJob;
class TestJob;
class OpenJob;
class OpenWithJob;
class Plugin;
class PreviewJob;
class Query;
class ReadOnlyArchiveInterface;

enum ArchiveError { NoError = 0, NoPlugin, FailedPlugin };

class KERFUFFLE_EXPORT Archive : public QObject
{
    Q_OBJECT

    /**
     *  Complete base name, without the "tar" extension (if any).
     */
    Q_PROPERTY(QString completeBaseName READ completeBaseName CONSTANT)
    Q_PROPERTY(QString fileName READ fileName CONSTANT)
    Q_PROPERTY(QString comment READ comment CONSTANT)
    Q_PROPERTY(QMimeType mimeType READ mimeType CONSTANT)
    Q_PROPERTY(bool isEmpty READ isEmpty)
    Q_PROPERTY(bool isReadOnly READ isReadOnly CONSTANT)
    Q_PROPERTY(bool isSingleFile READ isSingleFile)
    Q_PROPERTY(bool isSingleFolder MEMBER m_isSingleFolder READ isSingleFolder)
    Q_PROPERTY(bool isMultiVolume READ isMultiVolume WRITE setMultiVolume)
    Q_PROPERTY(bool numberOfVolumes READ numberOfVolumes)
    Q_PROPERTY(EncryptionType encryptionType MEMBER m_encryptionType READ encryptionType)
    Q_PROPERTY(uint numberOfEntries READ numberOfEntries)
    Q_PROPERTY(qulonglong unpackedSize MEMBER m_extractedFilesSize READ unpackedSize)
    Q_PROPERTY(qulonglong packedSize READ packedSize)
    Q_PROPERTY(QString subfolderName MEMBER m_subfolderName READ subfolderName)
    Q_PROPERTY(QString password READ password)
    Q_PROPERTY(QStringList compressionMethods MEMBER m_compressionMethods)
    Q_PROPERTY(QStringList encryptionMethods MEMBER m_encryptionMethods)

public:
    enum EncryptionType { Unencrypted, Encrypted, HeaderEncrypted };
    Q_ENUM(EncryptionType)

    class Entry;

    QString completeBaseName() const;
    QString fileName() const;
    QString comment() const;
    QMimeType mimeType();
    bool isEmpty() const;
    bool isReadOnly() const;
    bool isSingleFile() const;
    bool isSingleFolder() const;
    bool isMultiVolume() const;
    void setMultiVolume(bool value);
    bool hasComment() const;
    int numberOfVolumes() const;
    EncryptionType encryptionType() const;
    QString password() const;
    uint numberOfEntries() const;
    qulonglong unpackedSize() const;
    qulonglong packedSize() const;
    QString subfolderName() const;
    QString multiVolumeName() const;
    ReadOnlyArchiveInterface *interface();

    /**
     * @return Whether the archive has more than one top-level entry.
     */
    bool hasMultipleTopLevelEntries() const;

    /**
     * @return Batch extraction job for @p filename to @p destination.
     * @param autoSubfolder Whether the job will extract into a subfolder.
     * @param preservePaths Whether the job will preserve paths.
     * @param parent The parent for the archive.
     */
    static BatchExtractJob *
    batchExtract(const QString &fileName, const QString &destination, bool autoSubfolder, bool preservePaths, QObject *parent = nullptr);

    /**
     * @return Job to create an archive for the given @p entries.
     * @param fileName The name of the new archive.
     * @param mimeType The mimetype of the new archive.
     */
    static CreateJob *create(const QString &fileName,
                             const QString &mimeType,
                             const QList<Archive::Entry *> &entries,
                             const CompressionOptions &options,
                             QObject *parent = nullptr);

    /**
     * @return An empty archive with name @p fileName, mimetype @p mimeType and @p parent as parent.
     */
    static Archive *createEmpty(const QString &fileName, const QString &mimeType, QObject *parent = nullptr);

    /**
     * @return Job to load the archive @p fileName.
     * @param parent The parent of the archive that will be loaded.
     */
    static LoadJob *load(const QString &fileName, QObject *parent = nullptr);

    /**
     * @return Job to load the archive @p fileName with mimetype @p mimeType.
     * @param parent The parent of the archive that will be loaded.
     */
    static LoadJob *load(const QString &fileName, const QString &mimeType, QObject *parent = nullptr);

    /**
     * @return Job to load the archive @p fileName by using @p plugin.
     * @param parent The parent of the archive that will be loaded.
     */
    static LoadJob *load(const QString &fileName, Plugin *plugin, QObject *parent = nullptr);

    ~Archive() override;

    ArchiveError error() const;
    bool isValid() const;

    DeleteJob *deleteFiles(QList<Archive::Entry *> &entries);
    CommentJob *addComment(const QString &comment);
    TestJob *testArchive();

    AddJob *addFiles(const QList<Archive::Entry *> &files, const Archive::Entry *destination, const CompressionOptions &options = CompressionOptions());

    /**
     * Renames or moves entries within the archive.
     *
     * @param files All the renamed or moved files and their child entries (for renaming a directory too).
     * @param destination New entry name (for renaming) or destination folder (for moving).
     * If ReadOnlyArchiveInterface::entriesWithoutChildren(files).count() returns 1, then it's renaming,
     * so you must specify the resulted entry name, even if it's not going to be changed.
     * Otherwise (if count is more than 1) it's moving, so destination must contain only targeted folder path
     * or be empty, if moving to the root.
     */
    MoveJob *moveFiles(const QList<Archive::Entry *> &files, Archive::Entry *destination, const CompressionOptions &options = CompressionOptions());

    /**
     * Copies entries within the archive.
     *
     * @param files All the renamed or moved files and their child entries (for renaming a directory too).
     * @param destination Destination path. It must contain only targeted folder path or be empty,
     * if copying to the root.
     */
    CopyJob *copyFiles(const QList<Archive::Entry *> &files, Archive::Entry *destination, const CompressionOptions &options = CompressionOptions());

    ExtractJob *extractFiles(const QList<Archive::Entry *> &files, const QString &destinationDir, ExtractionOptions options = ExtractionOptions());

    PreviewJob *preview(Archive::Entry *entry);
    OpenJob *open(Archive::Entry *entry);
    OpenWithJob *openWith(Archive::Entry *entry);

    /**
     * @param password The password to encrypt the archive with.
     * @param encryptHeader Whether to encrypt also the list of files.
     */
    void encrypt(const QString &password, bool encryptHeader);

private Q_SLOTS:
    void onAddFinished(KJob *);
    void onUserQuery(Kerfuffle::Query *);
    void onCompressionMethodFound(const QString &method);
    void onEncryptionMethodFound(const QString &method);

private:
    Archive(ReadOnlyArchiveInterface *archiveInterface, bool isReadOnly, QObject *parent = nullptr);
    Archive(ArchiveError errorCode, QObject *parent = nullptr);

    static Archive *create(const QString &fileName, QObject *parent = nullptr);
    static Archive *create(const QString &fileName, const QString &fixedMimeType, QObject *parent = nullptr);

    /**
     * Create an archive instance from a given @p plugin.
     * @param fileName The name of the archive.
     * @return A valid archive if the plugin could be loaded, an invalid one otherwise (with the FailedPlugin error set).
     */
    static Archive *create(const QString &fileName, Plugin *plugin, QObject *parent = nullptr);
    ReadOnlyArchiveInterface *m_iface = nullptr;
    bool m_isReadOnly;
    bool m_isSingleFolder;
    bool m_isMultiVolume;

    QString m_subfolderName;
    qulonglong m_extractedFilesSize;
    ArchiveError m_error;
    EncryptionType m_encryptionType;
    QMimeType m_mimeType;
    QStringList m_compressionMethods;
    QStringList m_encryptionMethods;

    /**
     * @brief The user metadata for this archive.
     *
     * We keep track of the user metadata for the archive so that we can restore
     * it after the archive has been modified.
     */
    std::optional<MetadataBackup> m_userMetaData;

    /**
     * @brief Restore the user metadata for this archive.
     */
    void restoreUserMetadata();
};

} // namespace Kerfuffle

#endif // ARCHIVE_H
