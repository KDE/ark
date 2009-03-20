/*
 * Copyright (c) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (c) 2008 Harald Hvaal <haraldhv )@@@( stud(dot)ntnu.no>
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
#include "threading.h"

#include <kdebug.h>
#include <KLocale>
#include <QDir>
#include <QTimer>
#include <QApplication>

//#define KERFUFFLE_NOJOBTHREADING

namespace Kerfuffle
{

	Job::Job(ReadOnlyArchiveInterface *interface, QObject *parent)
		: KJob(NULL),
		m_interface(interface)
	{
		static bool onlyOnce = false;
		if (!onlyOnce) {
			qRegisterMetaType<QPair<QString, QString> >("QPair<QString,QString>");
			onlyOnce = true;
		}

		setCapabilities(KJob::Killable | KJob::Suspendable);
	}

	void Job::start()
	{
#ifdef KERFUFFLE_NOJOBTHREADING
		QTimer::singleShot(0, this, SLOT(doWork()));
#else
		ThreadExecution *thread = new ThreadExecution(this);

		//we want the event handling to happen in the work thread to avoid
		//unsafe data access due to events while work is being done
		moveToThread(thread);

		thread->start();
#endif
	}

	void Job::onError( const QString & message, const QString & details )
	{
		setError(1);
		setErrorText(message);
	}

	void Job::onEntry( const ArchiveEntry & archiveEntry )
	{
		emit newEntry( archiveEntry );
	}

	void Job::onProgress( double value )
	{
		setPercent( static_cast<unsigned long>( 100.0*value ) );
	}

	void Job::onInfo( const QString& info )
	{
		emit infoMessage(this, info);
	}

	void Job::onEntryRemoved( const QString & path )
	{
		emit entryRemoved( path );
	}

	ListJob::ListJob( ReadOnlyArchiveInterface *interface, QObject *parent )
		: Job( interface, parent ),
		m_isSingleFolderArchive(true),
		m_isPasswordProtected(false),
		m_extractedFilesSize(0)
	{
		connect(this, SIGNAL(newEntry( const ArchiveEntry&)),
				this, SLOT(onNewEntry( const ArchiveEntry&)));
	}

	void ListJob::doWork()
	{
		emit description( this, i18n( "Loading archive..." ) );
		m_interface->registerObserver( this );
		bool result = m_interface->list();
		m_interface->removeObserver( this );
		

		setError(!result);
		emitResult();

#ifndef KERFUFFLE_NOJOBTHREADING
		moveToThread(QApplication::instance()->thread());
#endif
	}

	void ListJob::onNewEntry(const ArchiveEntry& entry)
	{
		m_extractedFilesSize += entry[ Size ].toLongLong();
		m_isPasswordProtected |= entry [ IsPasswordProtected ].toBool();
		if (m_isSingleFolderArchive)
		{
			QString filename = entry[ FileName ].toString();
			if (m_previousEntry.isEmpty()) {
				//store the root path of the filename
				m_previousEntry = filename.split(QDir::separator()).first();
			}
			else {
				QString newRoot = filename.split(QDir::separator()).first();
				if (m_previousEntry != newRoot) {
					m_isSingleFolderArchive = false;
					m_subfolderName.clear();
				}
				else {
					m_previousEntry = newRoot;
					m_subfolderName = newRoot;
				}
			}
		}
	}

	ExtractJob::ExtractJob( const QList<QVariant>& files, const QString& destinationDir,
	                        ExtractionOptions options, ReadOnlyArchiveInterface *interface, QObject *parent )
		: Job(interface,  parent ), m_files( files ), m_destinationDir( destinationDir ), m_options(options)
	{
	}

	void ExtractJob::doWork()
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

		m_interface->registerObserver( this );

		fillInDefaultValues(m_options);

		kDebug(1601) << "Starting extraction with selected files "
			<< m_files
			<< " Destination dir " << m_destinationDir
			<< " And options " << m_options
					;

		setError( !m_interface->copyFiles( m_files, m_destinationDir, m_options ) );
		m_interface->removeObserver( this );

		emitResult();
#ifndef KERFUFFLE_NOJOBTHREADING
		moveToThread(QApplication::instance()->thread());
#endif

	}
	void ExtractJob::fillInDefaultValues(ExtractionOptions& options)
	{
		if (!options.contains("PreservePaths")) options["PreservePaths"] = false;
	}

	AddJob::AddJob( const QStringList& files, const CompressionOptions& options , ReadWriteArchiveInterface *interface, QObject *parent )
		: Job( interface, parent ), m_files( files ), m_options(options)
	{
		kDebug( 1601 );
	}

	void AddJob::doWork()
	{
		emit description( this, i18np( "Adding a file", "Adding %1 files", m_files.count() ) );

		ReadWriteArchiveInterface *m_writeInterface =
			qobject_cast<ReadWriteArchiveInterface*>
			(m_interface);

		Q_ASSERT(m_writeInterface);
		
		m_writeInterface->registerObserver( this );
		setError( !m_writeInterface->addFiles( m_files, m_options ) );
		m_writeInterface->removeObserver( this );

		emitResult();
#ifndef KERFUFFLE_NOJOBTHREADING
		moveToThread(QApplication::instance()->thread());
#endif

	}

	DeleteJob::DeleteJob( const QList<QVariant>& files, ReadWriteArchiveInterface *interface, QObject *parent )
		: Job( interface, parent ), m_files( files )
	{
	}

	void DeleteJob::doWork()
	{
		emit description( this, i18np( "Deleting a file from the archive", "Deleting %1 files", m_files.count() ) );

		ReadWriteArchiveInterface *m_writeInterface =
			qobject_cast<ReadWriteArchiveInterface*>
			(m_interface);

		Q_ASSERT(m_writeInterface);
		
		m_writeInterface->registerObserver( this );
		setError( !m_writeInterface->deleteFiles( m_files ) );
		m_writeInterface->removeObserver( this );

		emitResult();
#ifndef KERFUFFLE_NOJOBTHREADING
		moveToThread(QApplication::instance()->thread());
#endif
	}

} // namespace Kerfuffle
