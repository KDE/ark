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

#include "archiveentry.h"

#include <qdatetime.h>
#include <kmimetype.h>

ArchiveEntry::ArchiveEntry( const QString& path, Q_UINT64 size, const QDateTime& timeStamp )
  : m_path( path ), m_size( size ), m_timeStamp( timeStamp ),
    m_compressedSize( 0 ), m_crc( 0 )
{
}

ArchiveEntry::~ArchiveEntry()
{
}

QString ArchiveEntry::mimeType()
{
  if ( m_mimeType.isNull() )
  {
    m_mimeType = KMimeType::findByPath( m_path, 0, true )->name();
  }

  return m_mimeType;
}

float ArchiveEntry::compressionRatio() const
{
  return ( m_size == 0 ? 0.0 : 1.0 - ( float ) m_compressedSize/( float ) m_size );
}

