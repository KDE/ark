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

#include "archive.h"

#include <kactioncollection.h>

#include <qfileinfo.h>

Archive::Archive( const QString& archive, bool openReadOnly )
  : QObject(), m_fileName( archive ), m_readOnly( openReadOnly ),
    m_totalSize( 0 ), m_totalCompressedSize( 0 )
{
  QFileInfo fi( archive );
  // Open the archive in read-only mode if it is not writable or if openReadOnly is true
  m_readOnly = ( openReadOnly ) or ( !fi.isWritable() );

  // Creates the action collection
  m_actionCollection = new KActionCollection( 0, this );
  // initializes the actions
  initActions();
}

Archive::~Archive()
{
  // Delete the action collection.
  delete m_actionCollection;
  m_actionCollection = 0;
}

void Archive::initActions()
{
  // FIXME: Implement
}

void Archive::addEntry( const ArchiveEntry & entry )
{
  m_entries.append( entry );
  m_totalSize += entry.size();
  m_totalCompressedSize += entry.compressedSize();
  emit entryAdded( entry );
}

#include "archive.moc"
