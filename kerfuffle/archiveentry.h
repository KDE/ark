/*
    SPDX-FileCopyrightText: 2016 Vladyslav Batyrenko <mvlabat@gmail.com>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef ARCHIVEENTRY_H
#define ARCHIVEENTRY_H

#include "archive_kerfuffle.h"

#include <QDateTime>
#include <QIcon>

namespace Kerfuffle
{
enum PathFormat {
    NoTrailingSlash,
    WithTrailingSlash,
};

class KERFUFFLE_EXPORT Archive::Entry : public QObject
{
    Q_OBJECT

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
    Q_PROPERTY(QString fullPath MEMBER m_fullPath WRITE setFullPath)
    /// The internal name of the entry in the archive.
    Q_PROPERTY(QString name READ name)
    /// The visible name of the entry in the UI. This is usually (but not necessarily) equal to the name of the entry.
    Q_PROPERTY(QString displayName READ displayName WRITE setDisplayName)
    Q_PROPERTY(QString permissions MEMBER m_permissions)
    Q_PROPERTY(QString owner MEMBER m_owner)
    Q_PROPERTY(QString group MEMBER m_group)
    Q_PROPERTY(qulonglong size MEMBER m_size)
    Q_PROPERTY(qulonglong compressedSize MEMBER m_compressedSize)
    Q_PROPERTY(qulonglong sparseSize MEMBER m_sparseSize)
    Q_PROPERTY(QString link MEMBER m_link)
    Q_PROPERTY(QString ratio MEMBER m_ratio)
    Q_PROPERTY(QString CRC MEMBER m_CRC)
    Q_PROPERTY(QString BLAKE2 MEMBER m_BLAKE2)
    Q_PROPERTY(QString method MEMBER m_method)
    Q_PROPERTY(QString version MEMBER m_version)
    Q_PROPERTY(QDateTime timestamp MEMBER m_timestamp)
    Q_PROPERTY(bool isDirectory MEMBER m_isDirectory WRITE setIsDirectory)
    Q_PROPERTY(bool isExecutable MEMBER m_isExecutable WRITE setIsExecutable)
    Q_PROPERTY(bool isPasswordProtected MEMBER m_isPasswordProtected)
    Q_PROPERTY(bool isSparse MEMBER m_isSparse)

public:
    explicit Entry(QObject *parent = nullptr, const QString &fullPath = {}, const QString &rootNode = {});
    ~Entry() override;

    void copyMetaData(const Archive::Entry *sourceEntry);

    QList<Entry *> entries();
    const QList<Entry *> entries() const;
    void setEntryAt(int index, Entry *value);
    void appendEntry(Entry *entry);
    void removeEntryAt(int index);
    Entry *getParent() const;
    void setParent(Entry *parent);
    void setFullPath(const QString &fullPath);
    QString fullPath(PathFormat format = WithTrailingSlash) const;
    QString displayName() const;
    QString name() const;
    QStringView nameView() const;
    void setDisplayName(const QString &displayName);
    void setIsDirectory(const bool isDirectory);
    bool isDir() const;
    void setIsExecutable(const bool isExecutable);
    bool isExecutable() const;
    int row() const;
    Entry *find(QStringView name) const;
    Entry *findByPath(const QStringList &pieces, int index = 0) const;
    QIcon icon() const;
    qulonglong size() const;
    qulonglong sparseSize() const;
    bool isSparse() const;

    /**
     * Fills @p dirs and @p files with the number of directories and files
     * in the entry (both will be 0 if the entry is not a directory).
     */
    void countChildren(uint &dirs, uint &files) const;

    bool operator==(const Archive::Entry &right) const;

public:
    QString rootNode;
    bool compressedSizeIsSet;

private:
    QList<Entry *> m_entries;
    QString m_name;
    QString m_displayName;
    Entry *m_parent;

    QString m_fullPath;
    QString m_permissions;
    QString m_owner;
    QString m_group;
    qulonglong m_size;
    qulonglong m_compressedSize;
    qulonglong m_sparseSize;
    QString m_link;
    QString m_ratio;
    QString m_CRC;
    QString m_BLAKE2;
    QString m_method;
    QString m_version;
    QDateTime m_timestamp;
    bool m_isDirectory;
    bool m_isExecutable;
    bool m_isPasswordProtected;
    bool m_isSparse;
    mutable QIcon m_icon;
};

QDebug KERFUFFLE_EXPORT operator<<(QDebug d, const Kerfuffle::Archive::Entry &entry);
QDebug KERFUFFLE_EXPORT operator<<(QDebug d, const Kerfuffle::Archive::Entry *entry);

}

Q_DECLARE_METATYPE(Kerfuffle::Archive::Entry *)

#endif // ARCHIVEENTRY_H
