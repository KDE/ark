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

#include "archiveinterface.h"
#include "observer.h"
#include <kdebug.h>
#include <kfileitem.h>

#include <QFileInfo>
#include <QDir>

namespace Kerfuffle
{
	ReadOnlyArchiveInterface::ReadOnlyArchiveInterface( const QString & filename, QObject *parent )
		: QObject( parent ), m_filename( filename )
	{
	}

	ReadOnlyArchiveInterface::~ReadOnlyArchiveInterface()
	{
	}

	void ReadOnlyArchiveInterface::error( const QString & message, const QString & details )
	{
		foreach( ArchiveObserver *observer, m_observers )
		{
			observer->onError( message, details );
		}
	}

	void ReadOnlyArchiveInterface::entry( const ArchiveEntry & archiveEntry )
	{
		foreach( ArchiveObserver *observer, m_observers )
		{
			observer->onEntry( archiveEntry );
		}
	}

	void ReadOnlyArchiveInterface::entryRemoved( const QString & path )
	{
		foreach( ArchiveObserver *observer, m_observers )
		{
			observer->onEntryRemoved( path );
		}
	}

	void ReadOnlyArchiveInterface::progress( double p )
	{
		foreach( ArchiveObserver *observer, m_observers )
		{
			observer->onProgress( p );
		}
	}

	void ReadOnlyArchiveInterface::info( const QString& info)
	{
		foreach( ArchiveObserver *observer, m_observers )
		{
			observer->onInfo( info);
		}
	}

	void ReadOnlyArchiveInterface::registerObserver( ArchiveObserver *observer )
	{
		m_observers.append( observer );
	}

	void ReadOnlyArchiveInterface::removeObserver( ArchiveObserver *observer )
	{
		m_observers.removeAll( observer );
	}

	QString ReadOnlyArchiveInterface::findCommonBase(const QStringList& files)
	{
		QString commonBase;

		//we loop through all items and find the highest common folder they share
		if (files.size() > 1) {
			QStringList common = files.first().split('/', QString::SkipEmptyParts);
			if (common.size() > 1) {
				common.removeLast(); //We don't need the filename

				foreach(const QString &selectedEntry, files) {
					QStringList parts = selectedEntry.split('/', QString::SkipEmptyParts);
					for (int i = common.size() - 1; i > -1; --i) {
						if (common.at(i) != parts.at(i))
							common.removeLast();
					}
				}
				commonBase = common.join("/") + '/';
			}
		}
		else if (files.size() == 1) { 
			QStringList parts = files.first().split('/', QString::SkipEmptyParts);
			parts.removeLast(); //take of the filename
			return parts.join("/") + '/';
		}
		return commonBase;
	}

	QString ReadOnlyArchiveInterface::findCommonBase(const QVariantList& files)
	{
		QString commonBase;

		//we loop through all items and find the highest common folder they share
		if (files.size() > 1) {
			QStringList common = files.first().toString().split('/', QString::SkipEmptyParts);
			if (common.size() > 1) {
				common.removeLast(); //We don't need the filename

				foreach(const QVariant &selectedEntry, files) {
					QStringList parts = selectedEntry.toString().split('/', QString::SkipEmptyParts);
					for (int i = common.size() - 1; i > -1; --i) {
						if (common.at(i) != parts.at(i))
							common.removeLast();
					}
				}
				commonBase = common.join("/") + '/';
			}
		}
		else if (files.size() == 1) { 
			QStringList parts = files.first().toString().split('/', QString::SkipEmptyParts);
			parts.removeLast(); //take of the filename
			return parts.join("/") + '/';
		}
		return commonBase;
	}

	KJob* ReadOnlyArchiveInterface::listRecursiveTo(QString folder, QStringList& list)
	{
		return NULL;
	}

	void ReadOnlyArchiveInterface::expandDirectories( QStringList &files )
	{
		static bool onlyOnce = false;
		if (!onlyOnce) {
			qRegisterMetaType<KIO::filesize_t>("KIO::filesize_t");
			qRegisterMetaType<KIO::UDSEntryList>("KIO::UDSEntryList");
			onlyOnce = true;
		}

		for(int i = 0; i < files.size(); ++i) {
			const QString& item = files.at(i);
			if (QFileInfo(item).isDir()) {
				QString absolutePath = QFileInfo(item).absoluteFilePath();
				if (!absolutePath.endsWith('/')) absolutePath += '/';
				Q_ASSERT(QFileInfo(absolutePath).exists());
				kDebug( 1601 ) << "Calling listRecursive on " << absolutePath;
				KIO::ListJob *listJob = KIO::listRecursive(absolutePath, KIO::HideProgressInfo);
				RecursiveListHelper helper(absolutePath);
				connect(listJob, SIGNAL(entries (KIO::Job *, const KIO::UDSEntryList &)),
						&helper, SLOT(entries (KIO::Job *, const KIO::UDSEntryList &)));
				listJob->exec();

				foreach(const KFileItem& result, helper.results) {
					QString final = absolutePath + result.name();
					if (result.isDir() && !final.endsWith('/'))
						final += '/';
					files.insert(i + 1, final);
					++i;
				}
				
			}
		}
	}

	void RecursiveListHelper::entries (KIO::Job *job, const KIO::UDSEntryList &list)
	{
		foreach( const KIO::UDSEntry& entry, list) {
			KFileItem item(entry, m_listDir);

			QString value = entry.stringValue(KIO::UDSEntry::UDS_NAME);
			results.append(item);
		}
	}

	ReadWriteArchiveInterface::ReadWriteArchiveInterface( const QString & filename, QObject *parent )
		: ReadOnlyArchiveInterface( filename, parent )
	{
	}

	ReadWriteArchiveInterface::~ReadWriteArchiveInterface()
	{
	}

	bool ReadWriteArchiveInterface::isReadOnly() const
	{
		QFileInfo fileInfo( filename() );
		if ( fileInfo.exists() )
		{
			return ! fileInfo.isWritable();
		}
		else
		{
			return !fileInfo.dir().exists(); // TODO: Should also check if we can create a file in that directory
		}
	}

} // namespace Kerfuffle

#include "archiveinterface.moc"
