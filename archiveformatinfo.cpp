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

#include "arch.h"
#include "archiveformatinfo.h"

#include <klocale.h>
#include <kdebug.h>
#include <kmimetype.h>
#include <kdesktopfile.h>

#include <qfile.h>

ArchiveFormatInfo * ArchiveFormatInfo::m_pSelf = 0;

ArchiveFormatInfo::ArchiveFormatInfo()
    :m_lastExtensionUnknown( false )
{
    buildFormatInfos();
}

ArchiveFormatInfo * ArchiveFormatInfo::self()
{
    if ( !m_pSelf )
        m_pSelf = new ArchiveFormatInfo();
    return m_pSelf;
}

void ArchiveFormatInfo::buildFormatInfos()
{
  addFormatInfo( TAR_FORMAT, "application/x-tgz" );
  addFormatInfo( TAR_FORMAT, "application/x-tzo" );
  addFormatInfo( TAR_FORMAT, "application/x-tarz" );
  addFormatInfo( TAR_FORMAT, "application/x-tbz" );
  // x-tar as the last one to get its comment for all the others, too
  addFormatInfo( TAR_FORMAT, "application/x-tar" );

  addFormatInfo( LHA_FORMAT, "application/x-lha" );

  addFormatInfo( ZIP_FORMAT, "application/x-jar" );
  addFormatInfo( ZIP_FORMAT, "application/x-zip" );

  addFormatInfo( COMPRESSED_FORMAT, "application/x-gzip" );
  addFormatInfo( COMPRESSED_FORMAT, "application/x-bzip" );
  addFormatInfo( COMPRESSED_FORMAT, "application/x-bzip2" );
  addFormatInfo( COMPRESSED_FORMAT, "application/x-lzop" );
  addFormatInfo( COMPRESSED_FORMAT, "application/x-compress" );
  find( COMPRESSED_FORMAT ).description = i18n( "Compressed File" );

  addFormatInfo( ZOO_FORMAT, "application/x-zoo" );

  addFormatInfo( RAR_FORMAT, "application/x-rar" );
  addFormatInfo( AA_FORMAT, "application/x-archive" );
}

void ArchiveFormatInfo::addFormatInfo( ArchType type, QString mime )
{
    FormatInfo & info = find( type );

    KDesktopFile * desktopFile = new KDesktopFile( mime + ".desktop", true, "mime" );
    if( !desktopFile )
        kdWarning( 1601 ) << "MimeType " << mime << " seems to be missing." << endl;
    KMimeType mimeType( desktopFile );
    info.mimeTypes.append( mimeType.name() );
    info.extensions += mimeType.patterns();
    info.allDescriptions.append( mimeType.comment() );
    info.description = mimeType.comment();

    delete desktopFile;
}


QString ArchiveFormatInfo::filter()
{
    QStringList allExtensions;
    QString filter;
    InfoList::Iterator it;
    for ( it = m_formatInfos.begin(); it != m_formatInfos.end(); ++it )
    {
        allExtensions += (*it).extensions;
        filter += "\n" + (*it).extensions.join( " " ) + '|' + (*it).description;
    }
    return i18n( "*|All Files\n" )
                 + allExtensions.join( " " ) + '|' + i18n( "All Valid Archives" )
                 + filter;
}

QStringList ArchiveFormatInfo::allDescriptions()
{
    QStringList descriptions;
    InfoList::Iterator it;
    for ( it = m_formatInfos.begin(); it != m_formatInfos.end(); ++it )
        descriptions += (*it).allDescriptions;
    return descriptions;
}

ArchiveFormatInfo::FormatInfo & ArchiveFormatInfo::find( ArchType type )
{
    InfoList::Iterator it = m_formatInfos.begin();
    for( ; it != m_formatInfos.end(); ++it )
        if( (*it).type == type )
            return (*it);

    FormatInfo info;
    info.type = type;
    return ( *m_formatInfos.append( info ) );
}

ArchType ArchiveFormatInfo::archTypeByExtension( const QString & archname )
{
    InfoList::Iterator it = m_formatInfos.begin();
    QStringList::Iterator ext;
    for( ; it != m_formatInfos.end(); ++it )
    {
        ext = (*it).extensions.begin();
        for( ; ext != (*it).extensions.end(); ++ext )
            if( archname.endsWith( (*ext).remove( '*' ) ) )
                return (*it).type;
    }
    return UNKNOWN_FORMAT;
}

ArchType ArchiveFormatInfo::archTypeForMimeType( const QString & mimeType )
{
    InfoList::Iterator it = m_formatInfos.begin();
    for( ; it != m_formatInfos.end(); ++it )
    {
        int index = (*it).mimeTypes.findIndex( mimeType );
        if( index != -1 )
            return (*it).type;
    }
    return UNKNOWN_FORMAT;
}

ArchType ArchiveFormatInfo::archTypeForURL( const KURL & url )
{
    m_lastExtensionUnknown = false;

    if( url.isEmpty() )
        return UNKNOWN_FORMAT;

    if( !QFile::exists( url.path() ) )
        return archTypeByExtension( url.path() );

    QString mimeType = KMimeType::findByURL( url, 0, true, true )->name();
    kdDebug( 1601 ) << "find by url: " << mimeType << endl;
    if( mimeType == KMimeType::defaultMimeType() )
    {
        m_lastExtensionUnknown = true;
        mimeType = KMimeType::findByFileContent( url.path() )->name();
    }

    return archTypeForMimeType( mimeType );
}

QString ArchiveFormatInfo::mimeTypeForDescription( const QString & description )
{
    InfoList::Iterator it = m_formatInfos.begin();
    int index;
    for( ; it != m_formatInfos.end(); ++it )
    {
        index = (*it).allDescriptions.findIndex( description );
        if ( index != -1 )
            return (* (*it).mimeTypes.at( index ) );
    }
    return QString::null;
}

QString ArchiveFormatInfo::descriptionForMimeType( const QString & mimeType )
{
    InfoList::Iterator it = m_formatInfos.begin();
    int index;
    for( ; it != m_formatInfos.end(); ++it )
    {
        index = (*it).mimeTypes.findIndex( mimeType );
        if ( index != -1 )
            return (* (*it).allDescriptions.at( index ) );
    }
    return QString::null;
}

bool ArchiveFormatInfo::wasUnknownExtension()
{
    return m_lastExtensionUnknown;
}

