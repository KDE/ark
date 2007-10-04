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
#include "jobs.h"
#include "internaljobs.h"

#include <kdebug.h>
#include <KLocale>

namespace Kerfuffle
{
	ListJob::ListJob( ReadOnlyArchiveInterface *interface, QObject *parent )
		: KJob( parent ), m_archive( interface )
	{
	}

	void ListJob::start()
	{
		emit description( this, i18n( "Listing entries" ) );
		InternalListingJob *job = new InternalListingJob( m_archive, this );
		// TODO: connects
		connect( job, SIGNAL( entry( const ArchiveEntry& ) ),
		         this, SIGNAL( newEntry( const ArchiveEntry & ) ) );
		connect( job, SIGNAL( done( ThreadWeaver::Job* ) ),
		         this, SLOT( done( ThreadWeaver::Job* ) ) );
		connect( job, SIGNAL( progress( double ) ),
		         this, SLOT( progress( double ) ) );
		ThreadWeaver::Weaver::instance()->enqueue( job );
	}

	void ListJob::done( ThreadWeaver::Job *job )
	{
		emitResult();
	}

	ExtractJob::ExtractJob( const QList<QVariant>& files, const QString& destinationDir,
	                        bool preservePaths, ReadOnlyArchiveInterface *interface, QObject *parent )
		: KJob( parent ), m_files( files ), m_destinationDir( destinationDir ), m_preservePaths( preservePaths ),  m_archive( interface )
	{
	}

	void ExtractJob::start()
	{
		QString desc;
		if ( m_files.count() == 0 )
		{
			desc = i18n( "Extracting all files" );
		}
		else
		{
			desc = i18np( "Extracting one file", "Extracting %1 files", m_files.count() );
		}
		emit description( this, desc );
		InternalExtractJob *job = new InternalExtractJob( m_archive, m_files, m_destinationDir, m_preservePaths, this );

		connect( job, SIGNAL( done( ThreadWeaver::Job* ) ),
		         this, SLOT( done( ThreadWeaver::Job* ) ) );
		connect( job, SIGNAL( progress( double ) ),
		         this, SLOT( progress( double ) ) );
		connect( job, SIGNAL( error( const QString&, const QString& ) ),
		         this, SLOT( error( const QString&, const QString& ) ) );

		ThreadWeaver::Weaver::instance()->enqueue( job );
	}

	void ExtractJob::done( ThreadWeaver::Job *job )
	{
		emitResult();
	}

	void ExtractJob::progress( double p )
	{
		setPercent( static_cast<unsigned long>( 100.0*p ) );
	}

	void ExtractJob::error( const QString& errorMessage, const QString& details )
	{
		Q_UNUSED( details );
		setError( 1 );
		setErrorText( errorMessage );
	}

	void ListJob::progress( double p )
	{
		setPercent( static_cast<unsigned long>( 100.0*p ) );
	}

	AddJob::AddJob( const QStringList & files, ReadWriteArchiveInterface *interface, QObject *parent )
		: KJob( parent ), m_files( files ), m_archive( interface )
	{
	}

	void AddJob::start()
	{
		emit description( this, i18np( "Adding a file", "Adding %1 files", m_files.count() ) );
		
		InternalAddJob *job = new InternalAddJob( m_archive, m_files, this );
		
		connect( job, SIGNAL( done( ThreadWeaver::Job* ) ),
		         this, SLOT( done( ThreadWeaver::Job* ) ) );
		connect( job, SIGNAL( progress( double ) ),
		         this, SLOT( progress( double ) ) );
		connect( job, SIGNAL( entry( const ArchiveEntry& ) ),
		         this, SIGNAL( newEntry( const ArchiveEntry & ) ) );
		connect( job, SIGNAL( error( const QString&, const QString& ) ),
		         this, SLOT( error( const QString&, const QString& ) ) );
		
		ThreadWeaver::Weaver::instance()->enqueue( job );
	}

	void AddJob::done( ThreadWeaver::Job *job )
	{
		emitResult();
	}

	void AddJob::progress( double p )
	{
		setPercent( static_cast<unsigned long>( 100.0*p ) );
	}

	void AddJob::error( const QString& errorMessage, const QString& details )
	{
		kDebug( 1601 ) ;
		Q_UNUSED( details );
		setError( 1 );
		setErrorText( errorMessage );
	}

	DeleteJob::DeleteJob( const QList<QVariant>& files, ReadWriteArchiveInterface *interface, QObject *parent )
		: KJob( parent ), m_files( files ), m_archive( interface )
	{
	}

	void DeleteJob::start()
	{
		emit description( this, i18np( "Deleting a file from the archive", "Deleting %1 files", m_files.count() ) );

		InternalDeleteJob *job = new InternalDeleteJob( m_archive, m_files, this );

		connect( job, SIGNAL( done( ThreadWeaver::Job* ) ),
		         this, SLOT( done( ThreadWeaver::Job* ) ) );
		connect( job, SIGNAL( progress( double ) ),
		         this, SLOT( progress( double ) ) );
		connect( job, SIGNAL( entryRemoved( const QString& ) ),
		         this, SIGNAL( entryRemoved( const QString& ) ) );

		ThreadWeaver::Weaver::instance()->enqueue( job );
	}

	void DeleteJob::done( ThreadWeaver::Job *job )
	{
		emitResult();
	}

	void DeleteJob::progress( double p )
	{
		setPercent( static_cast<unsigned long>( 100.0*p ) );
	}

} // namespace Kerfuffle
