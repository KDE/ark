/*

 ark -- archiver for the KDE project

 Copyright (C) 2003 Georg Robbers <Georg.Robbers@urz.uni-hd.de>

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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#ifndef ARCHIVEFORMATINFO_H
#define ARCHIVEFORMATINFO_H

#include "arch.h"
#include <kmimetype.h>

class ArchiveFormatInfo
{
private:
    ArchiveFormatInfo();

public:
    static ArchiveFormatInfo * self();
    QString filter();
    const QStringList supportedMimeTypes( bool includeCompressed = true );
    QStringList allDescriptions();
    ArchType archTypeForMimeType( const QString & mimeType );
    ArchType archTypeByExtension( const QString & archname );
    ArchType archTypeForURL( const KURL & url );
    QString mimeTypeForDescription( const QString & description );
    QString findMimeType( const KURL & url );
    QString descriptionForMimeType( const QString & mimeType );
    QString defaultExtension( const QString & mimeType );
    bool wasUnknownExtension();

private:
    void buildFormatInfos();
    void addFormatInfo( ArchType type, QString mime, QString stdExt );

    struct FormatInfo
    {
        QStringList extensions;
        QStringList mimeTypes;
        QStringList allDescriptions;
        QStringList defaultExtensions;
        QString description;
        enum ArchType type;
    };

    FormatInfo & find ( ArchType type );

    typedef QValueList<FormatInfo> InfoList;
    InfoList m_formatInfos;

    bool m_lastExtensionUnknown;

    static ArchiveFormatInfo * m_pSelf;
};

#endif // ARCHIVEFORMATINFO_H

