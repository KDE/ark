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
#include <kdebug.h>

ListingJob::ListingJob( ReadOnlyArchiveInterface *archive, QObject *parent )
	: ThreadWeaver::Job( parent ), m_helper( 0 ), m_archive( archive ), m_success( false )
{
}

ListingJob::~ListingJob()
{
	delete m_helper;
	m_helper = 0;
}

void ListingJob::run()
{
	m_helper = new ListingJobHelper( m_archive );
	connect( m_helper, SIGNAL( entry( const ArchiveEntry & ) ),
	         this, SIGNAL( entry( const ArchiveEntry & ) ) );
	connect( m_helper, SIGNAL( progress( double ) ),
	         this, SIGNAL( progress( double ) ) );
	m_success = m_helper->getTheListing();
}

ListingJobHelper::ListingJobHelper( ReadOnlyArchiveInterface *archive, QObject *parent )
	: QObject( parent ), m_archive( archive )
{
}

ListingJobHelper::~ListingJobHelper()
{
}

bool ListingJobHelper::getTheListing()
{
	m_archive->registerObserver( this );
	bool result = m_archive->list();
	m_archive->removeObserver( this );
	return result;
}

void ListingJobHelper::onError( const QString & message, const QString & details )
{
	// TODO: do something
}

void ListingJobHelper::onEntry( const ArchiveEntry & archiveEntry )
{
	emit entry( archiveEntry );
}

void ListingJobHelper::onProgress( double d )
{
	emit progress( d );
}


void ListingJobHelper::entryslot( const ArchiveEntry & e )
{
	kDebug( 1601 ) << k_funcinfo << "Entry: " << e[ FileName ] << ", Owner = " << e[ Owner ] << endl;
}

#include "jobs.moc"
