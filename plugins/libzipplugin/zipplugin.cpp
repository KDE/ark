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

#include <QString>


#ifdef LIBZIP_COMPILED_WITH_32BIT_OFF_T

#define __off_t_defined
typedef quint32 off_t;

#endif /* LIBZIP_COMPILED_WITH_32BIT_OFF_T */


#include "kerfuffle/archiveinterface.h"
#include "kerfuffle/archivefactory.h"
#include <QDirIterator>

#include <zip.h>

#include <KLocale>
#include <KDebug>

#include <QDateTime>
#include <QFileInfo>
#include <QByteArray>
#include <QFile>
#include <QDir>

using namespace Kerfuffle;

class LibZipInterface: public ReadWriteArchiveInterface
{
	Q_OBJECT
	public:
		LibZipInterface( const QString & filename, QObject *parent )
			: ReadWriteArchiveInterface( filename, parent ), m_archive( 0 )
		{
		}

		~LibZipInterface()
		{
			kDebug( 1601 ) ;
			close();
		}

		bool open()
		{
			int errorCode;
			m_archive = zip_open( filename().toLocal8Bit(), ZIP_CREATE, &errorCode );
			if ( !m_archive )
			{
				error( i18n( "Could not open the archive '%1'", filename() ) );
				return false;
			}
			kDebug( 1601 ) << "Opened file " << filename();
			return true;
		}

		void close()
		{
			if ( m_archive )
			{
				zip_close( m_archive );
				m_archive = 0;
			}
		}

		void emitEntryForIndex( int index )
		{
			struct zip_stat stat;
			if ( zip_stat_index( m_archive, index, 0, &stat ) != 0 )
			{
				error( i18n( "An error occurred while trying to read entry #%1 of the archive", index ) );
				return;
			}

			QString filename = QDir::fromNativeSeparators(QFile::decodeName( stat.name ));
			bool isDirectory = (filename.right(1) == "/");
			QString crcHex;
			if (!isDirectory)
			{
				crcHex.sprintf("%08X", stat.crc);
			}

			ArchiveEntry e;

			e[ FileName ]       = filename;
			e[ InternalID ]     = filename;
			e[ CRC ]            = crcHex;
			e[ Size ]           = static_cast<qulonglong>( stat.size );
			e[ Timestamp ]      = QDateTime::fromTime_t( stat.mtime );
			e[ CompressedSize ] = static_cast<qulonglong>( stat.comp_size );
			e[ Method ]         = getCompressionMethodName(stat.comp_method);
			e[ IsPasswordProtected ] = stat.encryption_method? true : false;
			e[ IsDirectory ] = isDirectory;

			// TODO: zip_get_file_comment returns junk sometimes, find out why
			/*
			const char *comment = zip_get_file_comment( m_archive, index, 0, 0 );
			if ( comment )
			{
				e[ Comment ] = QString( comment );
			}
			*/

			entry( e );
		}

		// Returns the name of the compression method as defined in zip.h
		QString getCompressionMethodName(unsigned short method)
		{
			switch (method)
			{
				case ZIP_CM_DEFAULT:
					return "DEFAULT";
				case ZIP_CM_STORE:
					return "STORE";
				case ZIP_CM_SHRINK:
					return "SHRINK";
				case ZIP_CM_REDUCE_1:
					return "REDUCE_1";
				case ZIP_CM_REDUCE_2:
					return "REDUCE_2";
				case ZIP_CM_REDUCE_3:
					return "REDUCE_3";
				case ZIP_CM_REDUCE_4:
					return "REDUCE_4";
				case ZIP_CM_IMPLODE:
					return "IMPLODE";
				case ZIP_CM_DEFLATE:
					return "DEFLATE";

				// using constants because some of these may not be defined in older zip.h
				case 9: // ZIP_CM_DEFLATE64:
					return "DEFLATE64";
				case 10: // ZIP_CM_PKWARE_IMPLODE:
					return "PKWARE_IMPLODE";
				case 12: // ZIP_CM_BZIP2:
					return "BZIP2";
				case 14: // ZIP_CM_LZMA:
					return "LZMA";
				case 18: // ZIP_CM_TERSE:
					return "TERSE";
				case 19: // ZIP_CM_LZ77:
					return "LZ77";
				case 97: // ZIP_CM_WAVPACK:
					return "WAVPACK";
				case 98: // ZIP_CM_PPMD:
					return "PPMD";
				default:
					return QString("%1-UNKNOWN").arg(method);
			}
		}

		bool list()
		{
			kDebug( 1601 );
			if ( !open() ) // TODO: open should be called by the user, not by us
			{
				return false;
			}


			for ( int index = 0; index < zip_get_num_files( m_archive ); ++index )
			{
				emitEntryForIndex( index );
				progress( ( index+1 ) * 1.0/zip_get_num_files( m_archive ) );
			}
			close();
			return true;
		}

		QString destinationFileName( const QString& entryName, const QString& baseDir, bool preservePaths )
		{
			QString name = baseDir + '/';
			if ( preservePaths )
			{
				name += entryName;
			}
			else
			{
				name += QFileInfo( entryName ).fileName();
			}
			return name;
		}

		bool extractEntry(struct zip_file *file, QVariant entry, const QString & destinationDirectory, bool preservePaths , QString rootNode, QStringList& overwriteAllDirectories, QStringList& autoSkipDirectories, bool& userCancelled)
		{
			if (!rootNode.isEmpty()) {
				QString truncatedFilename;
				truncatedFilename = entry.toString().remove(0, rootNode.size());
				kDebug( 1601 ) << "Truncated filename: " << truncatedFilename;
				entry = truncatedFilename;
			}
			if (entry.toString().right(1) == "/") { // if a folder

				//if we don't preserve paths we don't create any folders
				if (!preservePaths) return true;

				if (!QDir(destinationDirectory).mkpath(entry.toString())) {
					error( i18n( "Could not create path" ) );
					zip_fclose( file );
					return false;
				}
				zip_fclose( file );
				return true;
			}

			QString destinationFilePath = destinationFileName( entry.toString(), destinationDirectory, preservePaths );

			if (!checkOverwrite(destinationFilePath, overwriteAllDirectories, autoSkipDirectories, userCancelled))
			{
				return (userCancelled) ? false : true;
			}

			// 2. Open the destination file
			QFile destinationFile( destinationFilePath );

			//create the path if it doesn't exist already
			if (preservePaths) {
				QDir dest(destinationDirectory);
				QFileInfo fi(destinationFile.fileName());
				if (!dest.exists(fi.path())) {
					dest.mkpath(fi.path());
				}
			}

			if ( !destinationFile.open( QIODevice::WriteOnly ) )
			{
				error( i18n( "Could not write to the destination file %1, path %2", entry.toString(), destinationFile.fileName()) );
				return false;
			}

			// 3. Copy the data
			char buffer[ 65536 ];
			int readBytes = -1;
			while ( ( readBytes = zip_fread( file, &buffer, 65536 ) ) != -1 )
			{
				if ( readBytes == 0 )
				{
					break;
				}
				destinationFile.write( buffer, readBytes );
			}

			// 4. Close the files
			zip_fclose( file );
			destinationFile.close();
			return true;
		}

		// TODO: too many repeated overwrite query codes, perhaps common them under Kerfuffle::OverwriteQuery?
		// Returns true if the file is to be overwritten
		bool checkOverwrite(QString & filename, QStringList& overwriteAllDirectories, QStringList& autoSkipDirectories, bool& userCancelled)
		{
			QFileInfo currentFileInfo(filename);

			if ( overwriteAllDirectories.contains(currentFileInfo.canonicalPath()) ||
				overwriteAllDirectories.contains(currentFileInfo.canonicalFilePath()))
			{
				return true;
			}

			if (autoSkipDirectories.contains(currentFileInfo.canonicalPath())
					|| autoSkipDirectories.contains(currentFileInfo.canonicalFilePath()))
			{
				return false;
			}

			if (currentFileInfo.exists())
			{
				Kerfuffle::OverwriteQuery query(filename);
				userQuery(&query);
				query.waitForResponse();
				if (query.responseRename())
				{
					filename = query.newFilename();
					return true;
				}
				if (query.responseOverwrite())
				{
					return true;
				}
				else if (query.responseAutoSkip())
				{
					if (currentFileInfo.isDir())
					{
						autoSkipDirectories << currentFileInfo.canonicalFilePath();
					}
					else
					{
						autoSkipDirectories << currentFileInfo.canonicalPath();
					}
					return false;
				}
				else if (query.responseOverwriteAll())
				{
					kDebug( 1601 ) << "adding overwrite all" << currentFileInfo.canonicalPath();
					overwriteAllDirectories << currentFileInfo.canonicalPath();
					return true;
				}
				else if (query.responseCancelled())
				{
					userCancelled = true;
					return false;
				}
				else // query.responseSkip()
				{
					return false;
				}
			}
			else
			{
				return true;
			}
		}

		bool copyFiles( const QList<QVariant> & files, const QString & destinationDirectory, ExtractionOptions options )
		{
			kDebug( 1601 ) ;

			const bool preservePaths = options.value("PreservePaths").toBool();

			QString rootNode;
			if (options.contains("RootNode"))
			{
				rootNode = options.value("RootNode").toString();
				kDebug(1601) << "Set root node " << rootNode;
			}

			if (!m_archive) {
				if (!open()) {
					return false;
				}
			}

			QStringList overwriteAllDirectories;
			QStringList autoSkipDirectories;
			bool userCancelled = false;
			int processed = 0;
			if (!files.isEmpty()) {
				///////////if only extract specified files
				foreach( const QVariant &entry, files )
				{

					// 1. Find the entry in the archive
					struct zip_file *file = zip_fopen( m_archive, QFile::encodeName(entry.toString()), 0 );
					if ( !file )
					{
						error( i18n( "Could not locate file '%1' in the archive", entry.toString() ) );
						return false;
					}

					if (!extractEntry(file, entry, destinationDirectory, preservePaths, rootNode, overwriteAllDirectories, autoSkipDirectories, userCancelled)) {
						return (userCancelled) ? true : false;
					}

					kDebug( 1601 ) << "Extracted " << entry.toString() ;

					progress( ( ++processed )*1.0/files.count() );
				}
			} else  {
				/////////////////if extract all files
				for ( int index = 0; index < zip_get_num_files( m_archive ); ++index )
				{

					// 1. Find the entry in the archive
					struct zip_file *file = zip_fopen_index( m_archive, index, 0 );
					if ( !file )
					{
						error( i18n( "Could not locate file #%1 in the archive", index ) );
						return false;
					}
					
					if (!extractEntry(file, QDir::fromNativeSeparators(QFile::decodeName(zip_get_name(m_archive, index, 0))), destinationDirectory, preservePaths, rootNode, overwriteAllDirectories, autoSkipDirectories, userCancelled)) {
						return (userCancelled) ? true : false;
					}

					kDebug( 1601 ) << "Extracted entry with index" << index ;

					progress( ( index+1 ) * 1.0/zip_get_num_files( m_archive ) );
				}
			}
			close();
			return true;
		}

		bool addFiles( const QStringList & files, const CompressionOptions& options )
		{
			kDebug( 1601 ) << "adding " << files.count() << " files";

			if (!m_archive) {
				if (!open()) {
					return false;
				}
			}

			QString globalWorkdir = options.value("GlobalWorkDir").toString();
			if (!globalWorkdir.isEmpty()) {
				kDebug( 1601 ) << "GlobalWorkDir is set, changing dir to " << globalWorkdir;
				QDir::setCurrent(globalWorkdir);
			}

			int processed = 0;
			foreach( const QString & file, files )
			{
				QString relativeName = QDir::current().relativeFilePath(file);
				kDebug(1601) << "file: " << file << relativeName;
				if (relativeName.isEmpty()) {
					//probably trying to add the current directory to the, with
					//the GlobalWorkdir set to the same value. 
					kDebug( 1601 ) << "Skipping empty relative-entry";
					continue;

				}

				if (QFileInfo(relativeName).isDir())
				{
					if (!relativeName.endsWith('/'))
						relativeName += '/';

					int result = writeFile(file, relativeName);
					if (result < 0) {
						kDebug( 1601 ) << "Error while compressing " << relativeName;
						return false;
					}

					QDirIterator it(file, QDirIterator::Subdirectories);

					while (it.hasNext()) {
						QString path = it.next();
						if (it.fileName() == ".." || it.fileName() == ".") continue;

						result = writeFile(path,
								QDir::current().relativeFilePath(path) + 
								(it.fileInfo().isDir() ? "/" : ""));
						if (result < 0) {
							kDebug( 1601 ) << "Error while compressing " << relativeName << it.fileName();
							return false;
						}

					}
				}
				else {
					int result = writeFile(file, relativeName);
					if (result < 0) {
						kDebug( 1601 ) << "Error while compressing " << relativeName;
						return false;
					}

				}

				if (files.count() > 1)
					progress( ( ++processed )*1.0/files.count() );
			}

			kDebug( 1601 ) << "And we're done :)" ;
			close();
			return true;
		}

		int writeFile(QString file, QString nameInArchive)
		{
			kDebug( 1601 ) << "Adding " << file  << " as " << nameInArchive;

			struct zip_source *source = zip_source_file( m_archive, QFile::encodeName(file).constData(), 0, -1 );

			if ( !source )
			{
				kDebug( 1601 ) << "Read error " << zip_strerror(m_archive);
				error( i18n( "Could not read from the input file '%1'", file ) );
				return -1;
			}


			int index;
			if (  ( index = zip_add( m_archive, QFile::encodeName(nameInArchive), source ) ) < 0 )
			{
				error( i18n( "Could not add the file %1 to the archive.", file) );
				return -1;
			}

			emitEntryForIndex( index );
			return index;

		}

		bool deleteFiles( const QList<QVariant> & files )
		{
			if (!m_archive)
				open();

			foreach( const QVariant& file, files )
			{
				int index = zip_name_locate( m_archive, file.toByteArray(), 0 );
				if ( index < 0 )
				{
					error( i18n( "Could not find a file named %1 in the archive.", file.toString() ) );
					return false;
				}
				zip_delete( m_archive, index );
				// TODO: emit some signal to inform the model of the deleted entry
				entryRemoved( file.toString() );
			}
			close();
			return true;
		}

	private:
		struct zip *m_archive;
};

#include "zipplugin.moc"

KERFUFFLE_PLUGIN_FACTORY( LibZipInterface )
