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

namespace Kerfuffle
{
	ListJob::ListJob( ReadOnlyArchiveInterface *interface, QObject *parent )
		: KJob( parent ), m_archive( interface )
	{
	}

	void ListJob::start()
	{
		InternalListingJob *job = new InternalListingJob( m_archive, this );
		// TODO: connects
		connect( job, SIGNAL( entry( const ArchiveEntry& ) ),
		         this, SIGNAL( newEntry( const ArchiveEntry & ) ) );
		connect( job, SIGNAL( done( ThreadWeaver::Job* ) ),
		         this, SLOT( done( ThreadWeaver::Job* ) ) );
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
		InternalExtractJob *job = new InternalExtractJob( m_archive, m_files, m_destinationDir, m_preservePaths, this );

		connect( job, SIGNAL( done( ThreadWeaver::Job* ) ),
		         this, SLOT( done( ThreadWeaver::Job* ) ) );

		ThreadWeaver::Weaver::instance()->enqueue( job );
	}

	void ExtractJob::done( ThreadWeaver::Job *job )
	{
		emitResult();
	}

} // namespace Kerfuffle
