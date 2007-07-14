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

#ifndef JOBS_H
#define JOBS_H

#include <ThreadWeaver/Job>
#include <ThreadWeaver/Weaver>

#include "archiveinterface.h"
#include <QList>

class ArchiveJobHelper;

class ListingJob: public ThreadWeaver::Job
{
	Q_OBJECT
	public:
		ListingJob( ReadOnlyArchiveInterface *archive, QObject *parent = 0 );
		~ListingJob();

		bool success() const { return m_success; }
	protected:
		void run();

	signals:
		void entry( const ArchiveEntry & );
		//void entries( const QList<ArchiveEntry & );
		void progress( double );
		void error( const QString& errorMessage, const QString& details );

	private:
		QList<ArchiveEntry>       m_entries;
		ArchiveJobHelper         *m_helper;
		ReadOnlyArchiveInterface *m_archive;
		bool                      m_success;
};

class ExtractionJob: public ThreadWeaver::Job
{
	Q_OBJECT
	public:
		ExtractionJob( ReadOnlyArchiveInterface *archive, const QList<QVariant> & files, const QString & destinationDirectory, QObject *parent = 0 );
		~ExtractionJob();

		bool success() const { return m_success; }

	protected:
		void run();

	signals:
		void progress( double p );
		void error( const QString& errorMessage, const QString& details );

	private:
		ReadOnlyArchiveInterface *m_archive;
		QList<QVariant>           m_files;
		QString                   m_destinationDirectory;
		ArchiveJobHelper         *m_helper;
		bool                      m_success;
};

#endif // JOBS_H
