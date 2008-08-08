/*
 * Copyright (c) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES ( INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION ) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * ( INCLUDING NEGLIGENCE OR OTHERWISE ) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "libarchivehandler.h"
//#include "settings.h"
#include "kerfuffle/archivefactory.h"

#include <archive.h>
#include <archive_entry.h>

#include <sys/stat.h>

#include <kdebug.h>

#include <QFile>
#include <QDir>
#include <QList>
#include <QStringList>
#include <QDateTime>

LibArchiveInterface::LibArchiveInterface( const QString & filename, QObject *parent )
	: ReadOnlyArchiveInterface( filename, parent ),
	cachedArchiveEntryCount(-1)
{
}

LibArchiveInterface::~LibArchiveInterface()
{
}

bool LibArchiveInterface::list()
{
	struct archive *arch;
	struct archive_entry *aentry;
	int result;

	arch = archive_read_new();
	if ( !arch )
		return false;

	result = archive_read_support_compression_all( arch );
	if ( result != ARCHIVE_OK ) return false;

	result = archive_read_support_format_all( arch );
	if ( result != ARCHIVE_OK ) return false;

	result = archive_read_open_filename( arch, QFile::encodeName( filename() ), 10240 );

	if ( result != ARCHIVE_OK )
	{
		error( QString( "Could not open the file '%1', libarchive cannot handle it." ).arg( filename() ), QString() );
		return false;
	}
	
	cachedArchiveEntryCount = 0;

	while ( ( result = archive_read_next_header( arch, &aentry ) ) == ARCHIVE_OK )
	{
		ArchiveEntry e;
		e[ FileName ] = QString( archive_entry_pathname( aentry ) );
		e[ InternalID ] = QByteArray( archive_entry_pathname( aentry ) );
		e[ Owner ] = QString( archive_entry_uname( aentry ) );
		e[ Group ] = QString( archive_entry_gname( aentry ) );
		e[ Size ] = ( qlonglong ) archive_entry_size( aentry );
		e[ IsDirectory ] = S_ISDIR( archive_entry_mode( aentry ) ); // see stat(2)
		if ( archive_entry_symlink( aentry ) )
		{
			e[ Link ] = archive_entry_symlink( aentry );
		}
		e[ Timestamp ] = QDateTime::fromTime_t( archive_entry_mtime( aentry ) );
		entry( e );
		archive_read_data_skip( arch );
		cachedArchiveEntryCount++;
	}

	if ( result != ARCHIVE_EOF )
	{
		return false;
	}

#if (ARCHIVE_API_VERSION>1)
	return archive_read_finish( arch ) == ARCHIVE_OK;
#else
	return true;
#endif
}

bool LibArchiveInterface::copyFiles( const QList<QVariant> & files, const QString & destinationDirectory, bool preservePaths )
{
	QDir::setCurrent( destinationDirectory );

	const bool extractAll = files.isEmpty();
	struct archive *arch, *writer;
	struct archive_entry *entry;

	QStringList entries;

	foreach( const QVariant &f, files )
	{
		entries << f.toString();
	}

	arch = archive_read_new();
	if ( !arch )
	{
		return false;
	}

	writer = archive_write_disk_new();
	archive_write_disk_set_options( writer, extractionFlags() );

	archive_read_support_compression_all( arch );
	archive_read_support_format_all( arch );
	int res = archive_read_open_filename( arch, QFile::encodeName( filename() ), 10240 );

	if ( res != ARCHIVE_OK )
	{
		kDebug( 1601 ) << "Couldn't open the file '" << filename() << "', libarchive can't handle it." ;
		return false;
	}

	int entryNr = 0, totalCount = -1;
	if (extractAll)
	{
		if (cachedArchiveEntryCount == -1)
		{
			progress(0);
			//TODO: once information progress has been implemented, send
			//feedback here that the archive is being read
			list();
		}
		totalCount = cachedArchiveEntryCount;
	}
	else
		totalCount = files.size();

	while ( archive_read_next_header( arch, &entry ) == ARCHIVE_OK )
	{
		QString entryName = QFile::decodeName( archive_entry_pathname( entry ) );
		if ( entries.contains( entryName ) || extractAll )
		{

			if( !preservePaths )
				archive_entry_set_pathname( entry, QFile::encodeName( QFileInfo( entryName ).fileName() ).constData() );

			if ( archive_write_header( writer, entry ) == ARCHIVE_OK )
				copyData( arch, writer );

			if (cachedArchiveEntryCount != -1)
			{
				++entryNr;
				progress(float(entryNr) / totalCount);
			}
			archive_entry_clear( entry );
			entries.removeAll( entryName );
		}
		else
		{
			archive_read_data_skip( arch );
		}
	}
	if ( entries.size() > 0 ) return false;

#if (ARCHIVE_API_VERSION>1)
	return archive_read_finish( arch ) == ARCHIVE_OK;
#else
	return true;
#endif
}


int LibArchiveInterface::extractionFlags() const
{
	int result = ARCHIVE_EXTRACT_TIME;
	result &= ARCHIVE_EXTRACT_SECURE_NODOTDOT;

	// TODO: Don't use arksettings here
	/*if ( ArkSettings::preservePerms() )
	{
		result &= ARCHIVE_EXTRACT_PERM;
	}

	if ( !ArkSettings::extractOverwrite() )
	{
		result &= ARCHIVE_EXTRACT_NO_OVERWRITE;
	}*/

	return result;
}

void LibArchiveInterface::copyData( struct archive *source, struct archive *dest )
{
	char buff[ARCHIVE_DEFAULT_BYTES_PER_BLOCK];
	ssize_t readBytes;

	readBytes = archive_read_data( source, buff, sizeof(buff) );
	while(readBytes > 0)
	{
	   /* int writeBytes = */ 
		archive_write_data( dest, buff, readBytes );
		if( archive_errno( dest ) != ARCHIVE_OK ) {
			kDebug() << "Error while extracting..." << archive_error_string( dest ) << "(error nb =" << archive_errno( dest ) << ')';
			return;
		}

		readBytes = archive_read_data( source, buff, sizeof(buff) );
	}
}

KERFUFFLE_PLUGIN_FACTORY( LibArchiveInterface )

#include "libarchivehandler.moc"
