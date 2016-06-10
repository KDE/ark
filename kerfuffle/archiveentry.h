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
    Q_PROPERTY(QVariant fileName MEMBER fileName)
    Q_PROPERTY(QVariant permissions MEMBER permissions)
    Q_PROPERTY(QVariant owner MEMBER owner)
    Q_PROPERTY(QVariant group MEMBER group)
    Q_PROPERTY(QVariant size MEMBER size)
    Q_PROPERTY(QVariant compressedSize MEMBER compressedSize)
    Q_PROPERTY(QVariant link MEMBER link)
    Q_PROPERTY(QVariant ratio MEMBER ratio)
    Q_PROPERTY(QVariant CRC MEMBER CRC)
    Q_PROPERTY(QVariant method MEMBER method)
    Q_PROPERTY(QVariant version MEMBER version)
    Q_PROPERTY(QVariant timestamp MEMBER timestamp)
    Q_PROPERTY(QVariant isDirectory MEMBER isDirectory)
    Q_PROPERTY(QVariant comment MEMBER comment)
    Q_PROPERTY(QVariant isPasswordProtected MEMBER isPasswordProtected)

public:

    Entry(Entry *parent);
    ~Entry();

    QList<Entry*> entries();
    void setEntryAt(int index, Entry *value);
    void appendEntry(Entry *entry);
    void removeEntryAt(int index);
    Entry *getParent() const;
    void setParent(Entry *parent);
    int row() const;
    bool isDir() const;
    void processNameAndIcon();
    QPixmap icon() const;
    QString name() const;
    Entry *find(const QString & name);
    Entry *findByPath(const QStringList & pieces, int index = 0);
    const QVariant &getPropertyByColumn(EntryMetaDataType column) const;
    void setPropertyByColumn(EntryMetaDataType column, const QVariant &value);
    void clearMetaData();
    void returnDirEntries(QList<Entry *> *store);
    void clear();

private:
    QList<Entry*>   m_entries;
    QPixmap         m_icon;
    QString         m_name;
    Entry           *m_parent;

public:
    QVariant fileName;
    QVariant permissions;
    QVariant owner;
    QVariant group;
    QVariant size;
    QVariant compressedSize;
    QVariant link;
    QVariant ratio;
    QVariant CRC;
    QVariant method;
    QVariant version;
    QVariant timestamp;
    QVariant isDirectory;
    QVariant comment;
    QVariant isPasswordProtected;
};

}


#endif //ARK_ENTRY_H
