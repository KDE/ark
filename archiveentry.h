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

#ifndef ARK_ARCHIVE_ENTRY_H
#define ARK_ARCHIVE_ENTRY_H

#include <qstring.h>
#include <qdatetime.h>

class ArchiveEntry
{
  public:
    ArchiveEntry( const QString& path, Q_UINT64 size, const QDateTime& timeStamp );
    ArchiveEntry() {}
    ~ArchiveEntry();

    QString   path() const { return m_path; }
    void setPath( const QString & path ) { m_path = path; }

    QString   mimeType();

    Q_UINT64  size() const { return m_size; }
    void setSize( Q_UINT64 size ) { m_size = size; }

    QDateTime timeStamp() const { return m_timeStamp; }
    void setTimeStamp( const QDateTime& timeStamp ) { m_timeStamp = timeStamp; }

    Q_UINT64 crc() const { return m_crc; }
    void setCRC( Q_UINT64 newCRC ) { m_crc = newCRC; }

    Q_UINT64 compressedSize() const { return m_compressedSize; }
    void setCompressedSize( Q_UINT64 compressedSize ) { m_compressedSize = compressedSize; }

    float compressionRatio() const;

  private:
    QString m_path;
    QString m_mimeType;
    Q_UINT64 m_size;
    QDateTime m_timeStamp;
    Q_UINT64 m_crc;
    Q_UINT64 m_compressedSize;
};

#endif //ARK_ARCHIVE_ENTRY_H
