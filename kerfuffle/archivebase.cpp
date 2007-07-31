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

#include "archivebase.h"
#include "internaljobs.h"

#include <kdebug.h>
#include <ThreadWeaver/Job>
#include <ThreadWeaver/Weaver>

#include <QFile>
#include <QDir>
#include <QList>
#include <QStringList>
#include <QDateTime>

namespace Kerfuffle
{
	ArchiveBase::ArchiveBase( ReadOnlyArchiveInterface *archive )
		: QObject(), Archive(), m_iface( archive )
	{
		Q_ASSERT( archive );
		archive->setParent( this );
		//setReadOnly( archive->isReadOnly() );
	}

	ArchiveBase::~ArchiveBase()
	{
		delete m_iface;
		m_iface = 0;
	}

	bool ArchiveBase::isReadOnly()
	{
		return m_iface->isReadOnly();
	}

	KJob* ArchiveBase::open()
	{
		return 0;
	}

	KJob* ArchiveBase::create()
	{
		return 0;
	}

	ListJob* ArchiveBase::list()
	{
		return new ListJob( m_iface, this );
	}

	DeleteJob* ArchiveBase::deleteFiles( const QList<QVariant> & files )
	{
		if ( m_iface->isReadOnly() )
		{
			return 0;
		}
		return new DeleteJob( files, static_cast<ReadWriteArchiveInterface*>( m_iface ), this );
	}

	AddJob* ArchiveBase::addFiles( const QStringList & files )
	{
		Q_ASSERT( !m_iface->isReadOnly() );
		return new AddJob( files, static_cast<ReadWriteArchiveInterface*>( m_iface ), this );
	}

	ExtractJob* ArchiveBase::copyFiles( const QList<QVariant> & files, const QString & destinationDir, bool preservePaths )
	{
		return new ExtractJob( files, destinationDir, preservePaths, m_iface, this );
	}

	QString ArchiveBase::fileName()
	{
		return m_iface->filename();
	}

} // namespace Kerfuffle

#include "archivebase.moc"
