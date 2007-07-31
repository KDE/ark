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

#include "kerfuffle_export.h"
#include "archiveinterface.h"
#include "archive.h"

#include <KJob>
#include <QList>
#include <QVariant>
#include <QString>

namespace ThreadWeaver
{
	class Job;
} // namespace ThreadWeaver

namespace Kerfuffle
{
	class KERFUFFLE_EXPORT ListJob: public KJob
	{
		Q_OBJECT
		public:
			explicit ListJob( ReadOnlyArchiveInterface *interface, QObject *parent = 0 );

			void start();

		signals:
			void newEntry( const ArchiveEntry & );
			void error( const QString& errorMessage, const QString& details );

		private slots:
			void done( ThreadWeaver::Job* );
			void progress( double );

		private:
			ReadOnlyArchiveInterface *m_archive;
	};

	class KERFUFFLE_EXPORT ExtractJob: public KJob
	{
		Q_OBJECT
		public:
			ExtractJob( const QList<QVariant> & files, const QString& destinationDir, bool preservePaths, ReadOnlyArchiveInterface *interface, QObject *parent = 0 );

			void start();

		private slots:
			void done( ThreadWeaver::Job * );
			void progress( double );
			void error( const QString&, const QString& );

		private:
			QList<QVariant>           m_files;
			QString                   m_destinationDir;
			bool                      m_preservePaths;
			ReadOnlyArchiveInterface *m_archive;
	};

	class KERFUFFLE_EXPORT AddJob: public KJob
	{
		Q_OBJECT
		public:
			AddJob( const QStringList & files, ReadWriteArchiveInterface *interface, QObject *parent = 0 );

			void start();

		signals:
			void newEntry( const ArchiveEntry & );

		private slots:
			void done( ThreadWeaver::Job * );
			void progress( double );
			void error( const QString&, const QString& );

		private:
			QStringList                m_files;
			ReadWriteArchiveInterface *m_archive;

	};

	class KERFUFFLE_EXPORT DeleteJob: public KJob
	{
		Q_OBJECT
		public:
			DeleteJob( const QList<QVariant>& files, ReadWriteArchiveInterface *interface, QObject *parent = 0 );

			void start();

		signals:
			void entryRemoved( const QString & entry );
			void error( const QString& errorMessage, const QString& details );

		private slots:
			void done( ThreadWeaver::Job * );
			void progress( double );

		private:
			QList<QVariant>            m_files;
			ReadWriteArchiveInterface *m_archive;
	};
} // namespace Kerfuffle

#endif // JOBS_H
