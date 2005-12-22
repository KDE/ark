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

#include "arch.h"
#include "archiveformatinfo.h"
#include "settings.h"

#include <klocale.h>
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kfilterdev.h>

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
  addFormatInfo( TAR_FORMAT, "application/x-tgz", ".tar.gz" );
  addFormatInfo( TAR_FORMAT, "application/x-tzo", ".tar.lzo" );
  addFormatInfo( TAR_FORMAT, "application/x-tarz", ".tar.z" );
  addFormatInfo( TAR_FORMAT, "application/x-tbz", ".tar.bz2" );
  addFormatInfo( TAR_FORMAT, "application/x-tbz2", ".tar.bz2" );
  // x-tar as the last one to get its comment for all the others, too
  addFormatInfo( TAR_FORMAT, "application/x-tar", ".tar" );

  addFormatInfo( LHA_FORMAT, "application/x-lha", ".lha" );

  addFormatInfo( ZIP_FORMAT, "application/x-jar", ".jar" );
  addFormatInfo( ZIP_FORMAT, "application/x-zip", ".zip" );
  addFormatInfo( ZIP_FORMAT, "application/x-zip-compressed", ".zip" );

  addFormatInfo( COMPRESSED_FORMAT, "application/x-gzip", ".gz" );
  addFormatInfo( COMPRESSED_FORMAT, "application/x-bzip", ".bz" );
  addFormatInfo( COMPRESSED_FORMAT, "application/x-bzip2", ".bz2" );
  addFormatInfo( COMPRESSED_FORMAT, "application/x-lzop", ".lzo"  );
  addFormatInfo( COMPRESSED_FORMAT, "application/x-compress", ".Z" );
  find( COMPRESSED_FORMAT ).description = i18n( "Compressed File" );

  addFormatInfo( ZOO_FORMAT, "application/x-zoo", ".zoo" );

  addFormatInfo( RAR_FORMAT, "application/x-rar", ".rar" );
  addFormatInfo( RAR_FORMAT, "application/x-rar-compressed", ".rar" );

  addFormatInfo( AA_FORMAT, "application/x-deb", ".deb" );
  addFormatInfo( AA_FORMAT, "application/x-archive",".a" );

  addFormatInfo( SEVENZIP_FORMAT, "application/x-7z", ".7z" );

  if ( ArkSettings::aceSupport() )
    addFormatInfo( ACE_FORMAT, "application/x-ace", ".ace" );
}

void ArchiveFormatInfo::addFormatInfo( ArchType type, QString mime, QString stdExt )
{
    FormatInfo & info = find( type );

    KDesktopFile * desktopFile = new KDesktopFile( mime + ".desktop", true, "mime" );
    if( !desktopFile )
        kdWarning( 1601 ) << "MimeType " << mime << " seems to be missing." << endl;
    KMimeType mimeType( desktopFile );
    info.mimeTypes.append( mimeType.name() );
    info.extensions += mimeType.patterns();
    info.defaultExtensions += stdExt;
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
    return allExtensions.join( " " ) + '|' + i18n( "All Valid Archives\n" )
            + "*|" + i18n( "All Files" )
            + filter;
}

const QStringList ArchiveFormatInfo::supportedMimeTypes( bool includeCompressed )
{
    QStringList list;

    InfoList::Iterator end = m_formatInfos.end();
    for ( InfoList::Iterator it = m_formatInfos.begin(); it != end; ++it )
    {
        if ( includeCompressed || ( *it ).type != COMPRESSED_FORMAT )
        {
		list += ( *it ).mimeTypes;
        }
    }

    return list;
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

    ArchType archType = archTypeForMimeType( mimeType );
    if ( archType == UNKNOWN_FORMAT )
        m_lastExtensionUnknown = true;

    return archType;
}


QString ArchiveFormatInfo::findMimeType( const KURL & url )
{
    QString mimeType = KMimeType::findByURL( url )->name();
    if ( mimeType != "application/x-bzip2" && mimeType != "application/x-gzip" )
        return mimeType;

    QIODevice * dev = KFilterDev::deviceForFile( url.path(), mimeType );
    if ( !dev )
        return mimeType;

    char buffer[ 0x200 ];

    dev->open(  IO_ReadOnly );
    Q_LONG n = dev->readBlock( buffer, 0x200 );
    delete dev;

    if ( n == 0x200 && buffer[0] != 0 && !strncmp(buffer + 257, "ustar", 5) )
    {
        if (mimeType == "application/x-bzip2")
            return "application/x-tbz";
        else
            return "application/x-tgz";
    }

    return mimeType;
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

QString ArchiveFormatInfo::defaultExtension( const QString & mimeType )
{
    InfoList::Iterator it = m_formatInfos.begin();
    int index;
    for( ; it != m_formatInfos.end(); ++it )
    {
        index = (*it).mimeTypes.findIndex( mimeType );
        if ( index != -1 )
            return (* (*it).defaultExtensions.at( index ) );
    }
    return QString::null;
}

bool ArchiveFormatInfo::wasUnknownExtension()
{
    return m_lastExtensionUnknown;
}

