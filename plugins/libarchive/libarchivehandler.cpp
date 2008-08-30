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
#include "kerfuffle/queries.h"

#include <archive.h>
#include <archive_entry.h>

#include <sys/stat.h>

#include <kdebug.h>
#include <KLocale>

#include <QFile>
#include <QDir>
#include <QList>
#include <QStringList>
#include <QDateTime>

LibArchiveInterface::LibArchiveInterface( const QString & filename, QObject *parent )
	: ReadWriteArchiveInterface( filename, parent ),
	cachedArchiveEntryCount(0),
	extractedFilesSize(0),
	overwriteAll(false)
{
}

LibArchiveInterface::~LibArchiveInterface()
{
}

void LibArchiveInterface::emitEntryFromArchiveEntry(struct archive_entry *aentry) {

	ArchiveEntry e;
	e[ FileName ] = QDir::fromNativeSeparators(QString::fromWCharArray(archive_entry_pathname_w( aentry )));
	e[ InternalID ] = e[ FileName ];
	e[ Owner ] = QString( archive_entry_uname( aentry ) );
	e[ Group ] = QString( archive_entry_gname( aentry ) );
	e[ Size ] = ( qlonglong ) archive_entry_size( aentry );
	e[ IsDirectory ] = S_ISDIR( archive_entry_mode( aentry ) ); // see stat(2)
	if ( archive_entry_symlink( aentry ) )
	{
		e[ Link ] = archive_entry_symlink( aentry );
	}
	e[ Timestamp ] = QDateTime::fromTime_t( archive_entry_mtime( aentry ) );

	entry(e);

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
		error( i18n( "Could not open the file '%1', libarchive cannot handle it." ).arg( filename() ), QString() );
		return false;
	}
	
	cachedArchiveEntryCount = 0;
	extractedFilesSize = 0;

	while ( ( result = archive_read_next_header( arch, &aentry ) ) == ARCHIVE_OK )
	{
		emitEntryFromArchiveEntry(aentry);
		extractedFilesSize += ( qlonglong ) archive_entry_size( aentry );

		cachedArchiveEntryCount++;
		archive_read_data_skip( arch );
	}


	if ( result != ARCHIVE_EOF )
	{
		error(i18n("The archive reading failed with message: %1").arg( archive_error_string(arch) ));
		return false;
	}

#if (ARCHIVE_API_VERSION>1)
	return archive_read_finish( arch ) == ARCHIVE_OK;
#else
	return true;
#endif
}

bool LibArchiveInterface::copyFiles( const QList<QVariant> & files, const QString & destinationDirectory, Archive::CopyFlags flags )
{
	QDir::setCurrent( destinationDirectory );


	const bool extractAll = files.isEmpty();
	const bool preservePaths = (flags & Archive::PreservePaths);
	overwriteAll = false; //we reset this per extract operation

	struct archive *arch, *writer;
	struct archive_entry *entry;
	
	QStringList entries;

	foreach( const QVariant &f, files ) {
		entries << f.toString();
	}

	QString commonBase;
	if (flags & Archive::TruncateCommonBase)
		commonBase = findCommonBase(files);

	//A trailing slash is very very very important here
	Q_ASSERT(commonBase.isEmpty() || commonBase.right(1) == "/");

	kDebug() << "Found common base " << commonBase;

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
		error(i18n("Couldn't open the file '%1', libarchive can't handle it.", filename())) ;
		return false;
	}

	int entryNr = 0, totalCount = 0;
	if (extractAll)
	{
		if (!cachedArchiveEntryCount)
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
	currentExtractedFilesSize = 0;

	while ( archive_read_next_header( arch, &entry ) == ARCHIVE_OK )
	{
		//we skip directories of not preserving paths
		if (!preservePaths && S_ISDIR(archive_entry_mode( entry ))) {
			archive_read_data_skip( arch );
			continue;
		}

		QString entryName = QDir::fromNativeSeparators(QFile::decodeName( archive_entry_pathname( entry ) ));

		if ( entries.contains( entryName ) || extractAll )
		{
			QFileInfo entryFI( entryName );

			QString fn = entryFI.fileName();
			QByteArray encodedFn = QFile::encodeName(fn);

			QByteArray encTruncatedFilename;
			QString truncatedFilename;

			if( !preservePaths ) {

				//empty filenames (ie dirs) should have been skipped already,
				//so asserting
				Q_ASSERT(!fn.isEmpty());

				archive_entry_set_pathname( entry, encodedFn.constData() );
				entryFI = QFileInfo(fn);

			} else if (!commonBase.isEmpty()) {
				truncatedFilename = entryName.remove(0, commonBase.size());
				kDebug( 1601 ) << "Truncated filename: " << truncatedFilename;
				encTruncatedFilename = QFile::encodeName(truncatedFilename);
				archive_entry_set_pathname( entry, encTruncatedFilename.constData() );

				entryFI = QFileInfo(truncatedFilename);
			}

			if (!overwriteAll) {
				if (entryFI.exists()) {
					Kerfuffle::OverwriteQuery query(entryName);
					emit userQuery(&query);
					query.waitForResponse();

					if (query.responseCancelled()) {
						entries.removeAll( entryName );
						archive_read_data_skip( arch );
						break;
					}

					if (query.responseOverwriteAll())
						overwriteAll = true;
				}
			}

			int header_response;
			kDebug(1601) << "Writing entry " << fn;
			if ( (header_response = archive_write_header( writer, entry )) == ARCHIVE_OK )
				//if the whole archive is extracted and the total filesize is
				//available, we use partial progress
				copyData( arch, writer, (extractAll && extractedFilesSize) ); 
			else {
				kDebug( 1601 ) << "Writing header failed with error code " << header_response
				<< "While attempting to write " << fn;
			}
			

			//if we only partially extract the archive and the number of
			//archive entries is available we use a simple progress based on
			//number of items extracted
			if (!extractAll && cachedArchiveEntryCount)
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

	archive_write_finish( writer );

#if (ARCHIVE_API_VERSION>1)
	return archive_read_finish( arch ) == ARCHIVE_OK;
#else
	return true;
#endif
}


int LibArchiveInterface::extractionFlags() const
{
	int result = ARCHIVE_EXTRACT_TIME;
	result |= ARCHIVE_EXTRACT_SECURE_NODOTDOT;

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

void LibArchiveInterface::copyData( QString filename, struct archive *dest, bool partialprogress )
{
	char buff[ARCHIVE_DEFAULT_BYTES_PER_BLOCK];
	ssize_t readBytes;
	QFile file(filename);

	if (!file.open(QIODevice::ReadOnly))
		return;

	readBytes = file.read( buff, sizeof(buff) );
	while(readBytes > 0)
	{
	   /* int writeBytes = */ 
		archive_write_data( dest, buff, readBytes );
		if( archive_errno( dest ) != ARCHIVE_OK ) {
			kDebug() << "Error while writing..." << archive_error_string( dest ) << "(error nb =" << archive_errno( dest ) << ')';
			return;
		}


		if (partialprogress) {
			currentExtractedFilesSize += readBytes;
			progress(float(currentExtractedFilesSize) / extractedFilesSize);
		}

		readBytes = file.read( buff, sizeof(buff) );
	}

	file.close();
}

void LibArchiveInterface::copyData( struct archive *source, struct archive *dest, bool partialprogress )
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


		if (partialprogress) {
			currentExtractedFilesSize += readBytes;
			progress(float(currentExtractedFilesSize) / extractedFilesSize);
		}

		readBytes = archive_read_data( source, buff, sizeof(buff) );
	}
}

bool LibArchiveInterface::addFiles(const QString& path, const QStringList & files )
{
	struct archive *arch_reader, *arch_writer;
	struct archive_entry *entry;
	int ret;
	int header_response;
	int destArchiveOpened = false;
	const bool creatingNewFile = !QFileInfo(filename()).exists();

	QString tempFilename = filename() + ".arkWriting";

	if (!creatingNewFile) {
		//*********initialize the reader
		arch_reader = archive_read_new();
		if ( !arch_reader )
		{
			error(i18n("The archive reader could not be initialized."));
			return false;
		}

		archive_read_support_compression_all( arch_reader );
		archive_read_support_format_all( arch_reader );
		ret = archive_read_open_filename( arch_reader, QFile::encodeName( filename() ), 10240 );
		if ( ret != ARCHIVE_OK )
		{
			error(i18n("The source file could not be read."));
			return false;
		}
	}

	//*********initialize the writer
	arch_writer = archive_write_new();
	if ( !arch_writer )
	{
		error(i18n("The archive writer could not be initialized."));
		return false;
	}

	if (creatingNewFile) {
		if (filename().right(2).toUpper() == "GZ") {
			kDebug(1601) << "Detected gzip compression for new file";
			ret = archive_write_set_compression_gzip(arch_writer);
		} else if (filename().right(3).toUpper() == "BZ2") {
			kDebug(1601) << "Detected bzip2 compression for new file";
			ret = archive_write_set_compression_bzip2(arch_writer);
		} else {
			kDebug(1601) << "Falling back to gzip";
			ret = archive_write_set_compression_gzip(arch_writer);
		}

		if (ret != ARCHIVE_OK) {
			error(i18n("Setting compression failed with the error '%1'", QString(archive_error_string( arch_writer ))));
			return false;
		}

		//pax_restricted is the libarchive default, let's go with that.
		archive_write_set_format_pax_restricted(arch_writer);

		if (ret != ARCHIVE_OK) {
			error(i18n("Setting format failed with the error '%1'", QString(archive_error_string( arch_writer ))));
			return false;
		}

		ret = archive_write_open_filename(arch_writer, QFile::encodeName( filename() ));
		if (ret != ARCHIVE_OK) {
			error(i18n("Opening the archive for writing failed with error message '%1'", QString(archive_error_string( arch_writer ))));
			return false;
		}

		destArchiveOpened = true;


	} else {
		switch (archive_compression(arch_reader)) {
			case ARCHIVE_COMPRESSION_GZIP:
				ret = archive_write_set_compression_gzip(arch_writer);
				break;
			case ARCHIVE_COMPRESSION_BZIP2:
				ret = archive_write_set_compression_bzip2(arch_writer);
				break;
			default:
				error(i18n("The compression type '%1' is not supported by Ark.", QString(archive_compression_name(arch_reader))));
				return false;
		}
		if (ret != ARCHIVE_OK) {
			error(i18n("Setting compression failed with the error '%1'", QString(archive_error_string( arch_writer ))));
			return false;
		}




		//********** copy all elements from previous archive to new archive
		while ( archive_read_next_header( arch_reader, &entry ) == ARCHIVE_OK )
		{

			if (!destArchiveOpened) {

				ret = archive_write_set_format(arch_writer, archive_format(arch_reader));

				//if setting the format did not succeed, try to fallback to the
				//base format
				if (ret != ARCHIVE_OK) {
					ret = archive_write_set_format(arch_writer, archive_format(arch_reader) & ARCHIVE_FORMAT_BASE_MASK);
				}

				if (ret != ARCHIVE_OK) {
					error(i18n("Setting format %2 failed with the error '%1'", QString(archive_error_string( arch_writer )), archive_format(arch_reader)));
					return false;
				}

				//we do not open the write archive before here because we need a
				//call to read_next_header for the right format to be detected
				ret = archive_write_open_filename(arch_writer, QFile::encodeName( tempFilename ));
				if (ret != ARCHIVE_OK) {
					error(i18n("Opening the archive for writing failed with error message '%1'", QString(archive_error_string( arch_writer ))));
					return false;
				}
				destArchiveOpened = true;
			}

			//kDebug(1601) << "Writing entry " << fn;
			if ( (header_response = archive_write_header( arch_writer, entry )) == ARCHIVE_OK )
				//if the whole archive is extracted and the total filesize is
				//available, we use partial progress
				copyData( arch_reader, arch_writer, false); 
			else {
				kDebug( 1601 ) << "Writing header failed with error code " << header_response;
				return false;
			}

			archive_entry_clear( entry );
		}
	}

	foreach(const QString& selectedFile, files) {

		struct stat st;
		QFileInfo info(selectedFile);
		entry = archive_entry_new();

		stat(QFile::encodeName(selectedFile).constData(), &st);
		archive_entry_copy_stat(entry, &st);
		archive_entry_copy_pathname( entry, QFile::encodeName(info.fileName()).constData() );

		kDebug( 1601 ) << "Writing new entry " << archive_entry_pathname(entry);
		if ( (header_response = archive_write_header( arch_writer, entry )) == ARCHIVE_OK )
			//if the whole archive is extracted and the total filesize is
			//available, we use partial progress
			copyData( selectedFile, arch_writer, false); 
		else {
			kDebug( 1601 ) << "Writing header failed with error code " << header_response;
			kDebug() << "Error while writing..." << archive_error_string( arch_writer ) << "(error nb =" << archive_errno( arch_writer ) << ')';
		}

		emitEntryFromArchiveEntry(entry);
		archive_entry_clear( entry );

	}

	ret = archive_write_finish(arch_writer);

	if (!creatingNewFile) {
		archive_read_finish( arch_reader );

		//everything seems OK, so we remove the source file and replace it with
		//the new one.
		//TODO: do some extra checks to see if this is really OK
		QFile::remove(filename());
		QFile::rename(tempFilename, filename());
	}

	return true;

}


bool LibArchiveInterface::deleteFiles( const QList<QVariant> & files )
{
	return false;
}

KERFUFFLE_PLUGIN_FACTORY( LibArchiveInterface )

#include "libarchivehandler.moc"
