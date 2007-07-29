#include "kerfuffle/archiveinterface.h"
#include "kerfuffle/archivefactory.h"

#include <zip.h>

#include <KLocale>
#include <KDebug>

#include <QDateTime>
#include <QString>
#include <QFileInfo>
#include <QByteArray>
#include <QFile>

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
			kDebug( 1601 ) << k_funcinfo << endl;
			if ( m_archive )
			{
				zip_close( m_archive );
				m_archive = 0;
			}
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
			return true;
		}

		void emitEntryForIndex( int index )
		{
			struct zip_stat stat;
			if ( zip_stat_index( m_archive, index, 0, &stat ) != 0 )
			{
				error( i18n( "An error occured while trying to read entry #%1 of the archive", index ) );
				return;
			}

			ArchiveEntry e;

			e[ FileName ]         = QString( stat.name );
			e[ OriginalFileName ] = QByteArray( stat.name );
			e[ CRC ]              = stat.crc;
			e[ Size ]             = static_cast<qulonglong>( stat.size );
			e[ Timestamp ]        = QDateTime::fromTime_t( stat.mtime );
			e[ CompressedSize ]   = static_cast<qulonglong>( stat.comp_size );
			e[ Method ]           = stat.comp_method;
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

		bool list()
		{
			if ( !open() ) // TODO: open should be called by the user, not by us
			{
				return false;
			}

			progress( 0.0 );

			for ( int index = 0; index < zip_get_num_files( m_archive ); ++index )
			{
				emitEntryForIndex( index );
				progress( ( index+1 ) * 1.0/zip_get_num_files( m_archive ) );
			}
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

		bool copyFiles( const QList<QVariant> & files, const QString & destinationDirectory, bool preservePaths )
		{
			kDebug( 1601 ) << k_funcinfo << endl;
			int processed = 0;
			foreach( const QVariant &entry, files )
			{
				kDebug( 1601 ) << "Trying to extract " << entry.toString() << endl;

				// 1. Find the entry in the archive
				struct zip_file *file = zip_fopen( m_archive, entry.toByteArray(), 0 );
				if ( !file )
				{
					error( i18n( "Could not locate file '%1' in the archive", entry.toString() ) );
					return false;
				}

				// 2. Open the destination file
				QFile destinationFile( destinationFileName( entry.toString(), destinationDirectory, preservePaths ) );
				if ( !destinationFile.open( QIODevice::WriteOnly ) )
				{
					error( i18n( "Could not write to the destination file" ) );
					return false;
				}

				// 3. Copy the data
				char buffer[ 65536 ];
				int readBytes = -1;
				while ( ( readBytes = zip_fread( file, &buffer, 65536 ) ) != -1 )
				{
					kDebug( 1601 ) << k_funcinfo << "Read " << readBytes << " bytes." << endl;
					if ( readBytes == 0 )
					{
						break;
					}
					destinationFile.write( buffer, readBytes );
				}

				// 4. Close the files
				zip_fclose( file );
				destinationFile.close();

				progress( ( ++processed )*1.0/files.count() );
			}
			return true;
		}

		bool addFiles( const QStringList & files )
		{
			kDebug( 1601 ) << k_funcinfo << "adding " << files.count() << " files"<< endl;
			progress( 0.0 );
			int processed = 0;
			foreach( const QString & file, files )
			{
				kDebug( 1601 ) << k_funcinfo << "Adding " << file << endl;
				QFileInfo fi( file );
				if ( fi.isDir() )
				{
					error( i18n( "Adding directories is not supported yet, sorry." ) );
					return false;
				}

				kDebug( 1601 ) << k_funcinfo << file << " is not a dir, good" << endl;

				struct zip_source *source = zip_source_file( m_archive, fi.absoluteFilePath().toLocal8Bit(), 0, -1 );
				if ( !source )
				{
					error( i18n( "Could not read from the input file '%1'", file ) );
					return false;
				}

				kDebug( 1601 ) << k_funcinfo << "We have a valid source for " << file << endl;

				int index;
				if (  ( index = zip_add( m_archive, fi.fileName().toLocal8Bit(), source ) ) < 0 )
				{
					error( i18n( "Could not add the file %1 to the archive.", fi.fileName() ) );
				}

				kDebug( 1601 ) << k_funcinfo << file << " was added to the archive, index is " << index << endl;

				emitEntryForIndex( index );
				progress( ( ++processed )*1.0/files.count() );
			}
			kDebug( 1601 ) << k_funcinfo << "And we're done :)" << endl;
			return true;
		}

		bool deleteFiles( const QList<QVariant> & files )
		{
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
			return true;
		}

	private:
		struct zip *m_archive;
};

#include "zipplugin.moc"

KERFUFFLE_PLUGIN_FACTORY( LibZipInterface );
