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

#include <kurl.h>

class KActionCollection;

class Archive : public QObject
{
  Q_OBJECT

  protected:
    /**
     * Constructor.
     * Protected, since it should not be called directly
     */
    Archive( const KURL& url, bool openReadOnly = false );

  public:
    /**
     * Destructor.
     */
    virtual ~Archive();

    /**
     * Returns true if the archive is read only, false otherwise.
     */
    bool readOnly() const { return m_readOnly; }

    /**
     * Returns a pointer to the KActionCollection object containing the 
     * archive's actions.
     */
    KActionCollection* actionCollection() const { return m_actionCollection; }

    /**
     * A list of entries in an archive.
     */
    typedef QValueList<ArchiveEntry> EntryList;

    /**
     * List of all entries in this archive.
     */
    EntryList entries() const { return m_entries; }

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
    /**
     * Creates the actions related to this archive.
     */
    void initActions();

    KURL m_url;
    bool m_readOnly;
    KActionCollection *m_actionCollection;
    EntryList m_entries;
};

#endif //ARK_ARCHIVE_H
