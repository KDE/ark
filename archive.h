/*

 ark -- archiver for the KDE project

 Copyright (c) 2005 by Henrique Pinto <henrique.pinto@kdemail.net>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef ARK_ARCHIVE_H
#define ARK_ARCHIVE_H

#include "archiveentry.h"

#include <qobject.h>
#include <qstring.h>

class KActionCollection;

class Archive : public QObject
{
  Q_OBJECT

  protected:
    /**
     * Constructor.
     * Protected, since it should not be called directly
     */
    Archive( const QString& archive, bool openReadOnly = false );

  public:
    /**
     * Destructor.
     */
    virtual ~Archive();

    /**
     * File name of the archive.
     * This class only deals with local archives, as KParts take care of URLs.
     */
    QString fileName() const { return m_fileName; }

    /**
     * Returns true if the archive is read only, false otherwise.
     */
    bool readOnly() const { return m_readOnly; }

    /**
     * A list of entries in an archive.
     */
    typedef QValueList<ArchiveEntry> EntryList;

    /**
     * List of all entries in this archive.
     */
    EntryList entries() const { return m_entries; }

    /**
     * Sum of the sizes of all entries in the archive.
     */
    Q_UINT64 totalSize() const { return m_totalSize; }

    /**
     * Sum of the sizes of all entries in the archive after the compression.
     */
    Q_UINT64 totalCompressedSize() const { return m_totalCompressedSize; }

  signals:
    /**
     * Emmited when an entry is going to be removed from the archive.
     */
    void entryAboutToBeRemoved( const ArchiveEntry & entry );

    /**
     * Emmited when an entry was added to the archive.
     */
    void entryAdded( const ArchiveEntry & entry );

  protected:
    /**
     * Forcefully make Ark treat the archive as read-only or not.
     */
    void setReadOnly( bool readOnly ) { m_readOnly = readOnly; }

    /**
     * Adds an entry to the archive.
     */
    void addEntry( const ArchiveEntry & entry );

  private:
    QString m_fileName;
    bool m_readOnly;
    Q_UINT64 m_totalSize;
    Q_UINT64 m_totalCompressedSize;
    EntryList m_entries;
};

#endif //ARK_ARCHIVE_H
