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
#include <memory>

KArchiveInterface::KArchiveInterface( const QString & filename, QObject *parent )
	: ReadOnlyArchiveInterface( filename, parent )
{
}

KArchiveInterface::~KArchiveInterface()
{
}

bool KArchiveInterface::list()
{
	std::auto_ptr<KArchive> archive( new KZip( filename() ) );

	if ( !archive->open( QIODevice::ReadOnly ) )
	{
		error( QString( "Couldn't open the archive '%1' for reading" ).arg( filename() ) );
		return false;
	}
	else
	{
		return browseArchive( archive.get() );
	}
}

bool KArchiveInterface::copyFiles( const QList<QVariant> & files, const QString & destinationDirectory )
{
	error( "Not implemented yet" );
	return false;
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
