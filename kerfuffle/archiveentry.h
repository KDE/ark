//
// Created by mvlabat on 5/27/16.
//

#ifndef ARK_ENTRY_H
#define ARK_ENTRY_H

#include "archive_kerfuffle.h"
#include "app/ark_debug.h"

#include <QtGui/QPixmap>
#include <QtCore/QMimeDatabase>
#include <QtCore/QDateTime>
#include <QtGui/QIcon>

#include <KIconLoader>

namespace Kerfuffle {
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
    Q_PROPERTY(QString method MEMBER m_method)
    Q_PROPERTY(QString version MEMBER m_version)
    Q_PROPERTY(QDateTime timestamp MEMBER m_timestamp)
    Q_PROPERTY(bool isDirectory MEMBER m_isDirectory WRITE setIsDirectory)
    Q_PROPERTY(QString comment MEMBER m_comment)
    Q_PROPERTY(bool isPasswordProtected MEMBER m_isPasswordProtected)

public:

    explicit Entry(QObject *parent = Q_NULLPTR, QString fullPath = QString(), QString rootNode = QString());
    ~Entry();

    void copyMetaData(const Archive::Entry *sourceEntry);

    QVector<Entry*> entries();
    const QVector<Entry*> entries() const;
    void setEntryAt(int index, Entry *value);
    void appendEntry(Entry *entry);
    void removeEntryAt(int index);
    Entry *getParent() const;
    void setParent(Entry *parent);
    void setFullPath(const QString &fullPath);
    QString fullPath(bool withoutTrailingSlash = false) const;
    QString name() const;
    void setIsDirectory(const bool isDirectory);
    bool isDir() const;
    int row() const;
    Entry *find(const QString &name) const;
    Entry *findByPath(const QStringList & pieces, int index = 0) const;
    void returnDirEntries(QList<Entry*> *store);
    void clear();

    bool operator==(const Archive::Entry &right) const;

public:
    QString rootNode;
    bool compressedSizeIsSet;

private:
    QVector<Entry*> m_entries;
    QString         m_name;
    Entry           *m_parent;

    QString m_fullPath;
    QString m_fullPathWithoutTrailingSlash;
    QString m_permissions;
    QString m_owner;
    QString m_group;
    qulonglong m_size;
    qulonglong m_compressedSize;
    QString m_link;
    QString m_ratio;
    QString m_CRC;
    QString m_method;
    QString m_version;
    QDateTime m_timestamp;
    bool m_isDirectory;
    QString m_comment;
    bool m_isPasswordProtected;
};

QDebug KERFUFFLE_EXPORT operator<<(QDebug d, const Kerfuffle::Archive::Entry &entry);
QDebug KERFUFFLE_EXPORT operator<<(QDebug d, const Kerfuffle::Archive::Entry *entry);

}

Q_DECLARE_METATYPE(Kerfuffle::Archive::Entry*)

#endif //ARK_ENTRY_H
