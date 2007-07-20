/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#include "karchiveplugin.h"
#include "kerfuffle/archivefactory.h"
#include <KZip>
#include <KTar>
#include <KMimeType>

KArchiveInterface::KArchiveInterface( const QString & filename, QObject *parent )
	: ReadOnlyArchiveInterface( filename, parent ), m_archive( 0 )
{
}

KArchiveInterface::~KArchiveInterface()
{
	delete m_archive;
	m_archive = 0;
}

KArchive *KArchiveInterface::archive()
{
	if ( m_archive == 0 )
	{
		KMimeType::Ptr mimeType = KMimeType::findByPath( filename() );

		if ( mimeType->is( "application/zip" ) )
		{
			m_archive = new KZip( filename() );
		}
		else
		{
			m_archive = new KTar( filename() );
		}

	}
	return m_archive;
}

bool KArchiveInterface::list()
{
	if ( !archive()->open( QIODevice::ReadOnly ) )
	{
		error( QString( "Couldn't open the archive '%1' for reading" ).arg( filename() ) );
		return false;
	}
	else
	{
		return browseArchive( archive() );
	}
}

bool KArchiveInterface::copyFiles( const QList<QVariant> & files, const QString & destinationDirectory, bool preservePaths )
{
	if ( preservePaths )
	{
		error( "Extraction preserving paths is not implemented yet." );
		return false;
	}
	foreach( const QVariant & file, files )
	{
		const KArchiveEntry *archiveEntry = archive()->directory()->entry( file.toString() );
		if ( !archiveEntry )
		{
			error( QString( "File '%1' not found in the archive" ).arg( file.toString() ) );
			return false;
		}

		// TODO: handle errors, copyTo fails silently
		if ( archiveEntry->isDirectory() )
		{
			static_cast<const KArchiveDirectory*>( archiveEntry )->copyTo( destinationDirectory );
		}
		else
		{
			static_cast<const KArchiveFile*>( archiveEntry )->copyTo( destinationDirectory );
		}
	}

	return true;
}

bool KArchiveInterface::browseArchive( KArchive *archive )
{
	return processDir( archive->directory() );
}

bool KArchiveInterface::processDir( const KArchiveDirectory *dir, const QString & prefix )
{
	foreach( const QString& entryName, dir->entries() )
	{
		const KArchiveEntry *entry = dir->entry( entryName );
		createEntryFor( entry, prefix );
		if ( entry->isDirectory() )
		{
			QString newPrefix = ( prefix.isEmpty()? prefix : prefix + '/' ) + entryName;
			processDir( static_cast<const KArchiveDirectory*>( entry ), newPrefix );
		}
	}
	return true;
}

void KArchiveInterface::createEntryFor( const KArchiveEntry *aentry, const QString& prefix )
{
	ArchiveEntry e;
	e[ FileName ]         = prefix.isEmpty()? aentry->name() : prefix + '/' + aentry->name();
	e[ OriginalFileName ] = e[ FileName ];
	e[ Permissions ]      = aentry->permissions();
	e[ Owner ]            = aentry->user();
	e[ Group ]            = aentry->group();
	e[ IsDirectory ]      = aentry->isDirectory();
	e[ Timestamp ]        = aentry->datetime();
	if ( !aentry->symlink().isEmpty() )
	{
		e[ Link ]             = aentry->symlink();
	}
	if ( aentry->isFile() )
	{
		e[ Size ] = static_cast<const KArchiveFile*>( aentry )->size();
	}
	entry( e );
}

KERFUFFLE_PLUGIN_FACTORY( KArchiveInterface );
