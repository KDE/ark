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
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef ARCHIVEFORMATINFO_H
#define ARCHIVEFORMATINFO_H

#include "arch.h"

class ArchiveFormatInfo
{
public:
    static QString filter();
    static QStringList allDescriptions();
    static ArchType archTypeForMimeType( const QString & mimeType );
    static ArchType archTypeByExtension( const QString & archname );
    static ArchType archTypeForURL( const KURL & url );
    static QString mimeTypeForDescription( const QString & description );
    static QString descriptionForMimeType( const QString & mimeType );
    static bool wasUnknownExtension();

private:
    static void buildFormatInfos();
    static void addFormatInfo( ArchType type, QString mime );

    struct FormatInfo
    {
        QStringList extensions;
        QStringList mimeTypes;
        QStringList allDescriptions;
        QString description;
        enum ArchType type;
    };

    static FormatInfo & find ( ArchType type );

    typedef QValueList<FormatInfo> InfoList;
    static InfoList m_formatInfos;

    static bool m_lastExtensionUnknown;
};

#endif // ARCHIVEFORMATINFO_H

