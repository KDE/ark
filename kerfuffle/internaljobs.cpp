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

#include "internaljobs.h"
#include <kdebug.h>


namespace Kerfuffle
{
	InternalJob::InternalJob( QObject *parent )
		: ThreadWeaver::Job( parent ), m_success( false )
	{
	}

	InternalJob::~InternalJob()
	{
	}

	InternalListingJob::InternalListingJob( ReadOnlyArchiveInterface *archive, QObject *parent )
		: InternalJob( parent ), m_helper( 0 ), m_archive( archive )
	{
	}

	InternalListingJob::~InternalListingJob()
	{
		delete m_helper;
		m_helper = 0;
	}

	void InternalListingJob::run()
	{
		m_helper = new ArchiveJobHelper( m_archive );
		connect( m_helper, SIGNAL( entry( const ArchiveEntry & ) ),
			 this, SIGNAL( entry( const ArchiveEntry & ) ) );
		connect( m_helper, SIGNAL( progress( double ) ),
			 this, SIGNAL( progress( double ) ) );
		connect( m_helper, SIGNAL( error( const QString&, const QString& ) ),
			 this, SIGNAL( error( const QString&, const QString& ) ) );
		setSuccess( m_helper->getTheListing() );
	}

	InternalExtractJob::InternalExtractJob( ReadOnlyArchiveInterface *archive, const QList<QVariant> & files, const QString & destinationDirectory, bool preservePaths, QObject *parent )
		: InternalJob( parent ), m_archive( archive ), m_files( files ), m_destinationDirectory( destinationDirectory ),
		  m_helper( 0 ), m_preservePaths( preservePaths )
	{

	}

	InternalExtractJob::~InternalExtractJob()
	{
		delete m_helper;
		m_helper = 0;
	}

	void InternalExtractJob::run()
	{
		m_helper = new ArchiveJobHelper( m_archive );
		connect( m_helper, SIGNAL( progress( double ) ),
			 this, SIGNAL( progress( double ) ) );
		connect( m_helper, SIGNAL( error( const QString&, const QString& ) ),
			 this, SIGNAL( error( const QString&, const QString& ) ) );
		m_archive->registerObserver( m_helper );
		setSuccess( m_archive->copyFiles( m_files, m_destinationDirectory, m_preservePaths ) );
		m_archive->removeObserver( m_helper );
	}

	InternalAddJob::InternalAddJob( ReadWriteArchiveInterface *archive, const QStringList & files, QObject *parent )
		: InternalJob( parent ), m_files( files ), m_archive( archive ), m_helper( 0 )
	{
	}

	InternalAddJob::~InternalAddJob()
	{
		delete m_helper;
		m_helper = 0;
	}

	void InternalAddJob::run()
	{
		m_helper = new ArchiveJobHelper( m_archive );

		connect( m_helper, SIGNAL( entry( const ArchiveEntry & ) ),
			 this, SIGNAL( entry( const ArchiveEntry & ) ) );
		connect( m_helper, SIGNAL( progress( double ) ),
			 this, SIGNAL( progress( double ) ) );
		connect( m_helper, SIGNAL( error( const QString&, const QString& ) ),
			 this, SIGNAL( error( const QString&, const QString& ) ) );

		m_archive->registerObserver( m_helper );
		setSuccess( m_archive->addFiles( m_files ) );
		m_archive->removeObserver( m_helper );
	}

	InternalDeleteJob::InternalDeleteJob( ReadWriteArchiveInterface *archive, const QList<QVariant> & entries, QObject *parent )
		: InternalJob( parent ), m_entries( entries ), m_archive( archive ), m_helper( 0 )
	{
	}

	InternalDeleteJob::~InternalDeleteJob()
	{
		delete m_helper;
		m_helper = 0;
	}

	void InternalDeleteJob::run()
	{
		m_helper = new ArchiveJobHelper( m_archive );

		// TODO: Connect the signals
		connect( m_helper, SIGNAL( entryRemoved( const QString& ) ),
		         this, SIGNAL( entryRemoved( const QString& ) ) );
		connect( m_helper, SIGNAL( progress( double ) ),
			 this, SIGNAL( progress( double ) ) );
		connect( m_helper, SIGNAL( error( const QString&, const QString& ) ),
			 this, SIGNAL( error( const QString&, const QString& ) ) );

		m_archive->registerObserver( m_helper );
		setSuccess( m_archive->deleteFiles( m_entries ) );
		m_archive->removeObserver( m_helper );
	}

	ArchiveJobHelper::ArchiveJobHelper( ReadOnlyArchiveInterface *archive, QObject *parent )
		: QObject( parent ), m_archive( archive )
	{
	}

	ArchiveJobHelper::~ArchiveJobHelper()
	{
	}

	bool ArchiveJobHelper::getTheListing()
	{
		m_archive->registerObserver( this );
		bool result = m_archive->list();
		m_archive->removeObserver( this );
		return result;
	}

	void ArchiveJobHelper::onError( const QString & message, const QString & details )
	{
		emit error( message, details );
	}

	void ArchiveJobHelper::onEntry( const ArchiveEntry & archiveEntry )
	{
		emit entry( archiveEntry );
	}

	void ArchiveJobHelper::onProgress( double d )
	{
		emit progress( d );
	}

	void ArchiveJobHelper::onEntryRemoved( const QString & path )
	{
		emit entryRemoved( path );
	}


} // namespace Kerfuffle

#include "internaljobs.moc"
