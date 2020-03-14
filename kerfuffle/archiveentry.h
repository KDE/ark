/*
 * Copyright (c) 2016 Vladyslav Batyrenko <mvlabat@gmail.com>
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

#ifndef ARCHIVEENTRY_H
#define ARCHIVEENTRY_H

#include "archive_kerfuffle.h"

#include <QDateTime>
#include <QIcon>

namespace Kerfuffle {

enum PathFormat {
    NoTrailingSlash,
    WithTrailingSlash
};

class Archive::Entry : public QObject
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
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QString permissions MEMBER m_permissions)
    Q_PROPERTY(QString owner MEMBER m_owner)
    Q_PROPERTY(QString group MEMBER m_group)
    Q_PROPERTY(qulonglong size MEMBER m_size)
    Q_PROPERTY(qulonglong compressedSize MEMBER m_compressedSize)
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

public:

    explicit Entry(QObject *parent = nullptr, const QString &fullPath = {}, const QString &rootNode = {});
    ~Entry() override;

    void copyMetaData(const Archive::Entry *sourceEntry);

    QVector<Entry*> entries();
    const QVector<Entry*> entries() const;
    void setEntryAt(int index, Entry *value);
    void appendEntry(Entry *entry);
    void removeEntryAt(int index);
    Entry *getParent() const;
    void setParent(Entry *parent);
    void setFullPath(const QString &fullPath);
    QString fullPath(PathFormat format = WithTrailingSlash) const;
    QString name() const;
    void setIsDirectory(const bool isDirectory);
    bool isDir() const;
    void setIsExecutable(const bool isExecutable);
    bool isExecutable() const;
    int row() const;
    Entry *find(const QString &name) const;
    Entry *findByPath(const QStringList & pieces, int index = 0) const;
    QIcon icon() const;

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
    QVector<Entry*> m_entries;
    QString         m_name;
    Entry           *m_parent;

    QString m_fullPath;
    QString m_permissions;
    QString m_owner;
    QString m_group;
    qulonglong m_size;
    qulonglong m_compressedSize;
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
    mutable QIcon m_icon;
};

QDebug KERFUFFLE_EXPORT operator<<(QDebug d, const Kerfuffle::Archive::Entry &entry);
QDebug KERFUFFLE_EXPORT operator<<(QDebug d, const Kerfuffle::Archive::Entry *entry);

}

Q_DECLARE_METATYPE(Kerfuffle::Archive::Entry*)

#endif //ARCHIVEENTRY_H
