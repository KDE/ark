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

#ifndef KERFUFFLE_INTERNAL_JOBS_H
#define KERFUFFLE_INTERNAL_JOBS_H

#include <ThreadWeaver/Job>
#include <ThreadWeaver/Weaver>

#include "archiveinterface.h"
#include "observer.h"
#include <QList>


namespace Kerfuffle
{
	class ArchiveJobHelper;

	class InternalListingJob: public ThreadWeaver::Job
	{
		Q_OBJECT
		public:
			InternalListingJob( ReadOnlyArchiveInterface *archive, QObject *parent = 0 );
			~InternalListingJob();

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

	class InternalExtractJob: public ThreadWeaver::Job
	{
		Q_OBJECT
		public:
			InternalExtractJob( ReadOnlyArchiveInterface *archive, const QList<QVariant> & files, const QString & destinationDirectory, bool preservePaths = false, QObject *parent = 0 );
			~InternalExtractJob();

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
			bool                      m_preservePaths;
	};

	class ArchiveJobHelper: public QObject, public ArchiveObserver
	{
		Q_OBJECT
		public:
			ArchiveJobHelper( ReadOnlyArchiveInterface *archive, QObject *parent = 0 );
			~ArchiveJobHelper();

			bool getTheListing();

			void onError( const QString & message, const QString & details = QString() );
			void onEntry( const ArchiveEntry & archiveEntry );
			void onProgress( double );

		signals:
			void entry( const ArchiveEntry & );
			void progress( double );
			void error( const QString & message, const QString & details );

		private slots:
				void entryslot( const ArchiveEntry & );

		private:
			ReadOnlyArchiveInterface *m_archive;
	};

} // namespace Kerfuffle

#endif // KERFUFFLE_INTERNAL_JOBS_H
