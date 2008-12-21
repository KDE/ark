/* * ark -- archiver for the KDE project
 *
 * Copyright (C) 2008 Harald Hvaal <haraldhv )attt( stud.ntnu.no>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "recursivelister.h"

#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kdebug.h>

#define KIO_IS_NOT_THREADSAFE

namespace Kerfuffle
{
	RecursiveLister::RecursiveLister(const QString& listDirectory)
		: m_listDirectory(listDirectory),
		m_entryIndex(0)
	{
		kDebug( 1601 );
	}
	
	void RecursiveLister::run()
	{
		qRegisterMetaType<KIO::filesize_t>("KIO::filesize_t");
		qRegisterMetaType<KIO::UDSEntryList>("KIO::UDSEntryList");

		kDebug( 1601 ) << m_listDirectory;
		m_listJob = KIO::listRecursive(m_listDirectory, KIO::HideProgressInfo);
		connect(m_listJob, SIGNAL(entries (KIO::Job *, const KIO::UDSEntryList &)),
				this, SLOT(slotEntry (KIO::Job *, const KIO::UDSEntryList &)),
				Qt::DirectConnection);
		connect(this, SIGNAL(finished()),
				this, SLOT(wakeWaiters()),
				Qt::DirectConnection);

		m_listJob->exec();
		kDebug (1601) << "finished";
	}

	void RecursiveLister::slotEntry(KIO::Job *job, const KIO::UDSEntryList& list)
	{
		kDebug (1601);
		foreach( const KIO::UDSEntry& entry, list) {
			KFileItem item(entry, m_listDirectory);
			if (item.name() == ".." || item.name() ==  ".") continue;
			m_entries.push_back(item);
			kDebug (1601) << item.localPath() + item.name();
		}
		wakeWaiters();
	}

	void RecursiveLister::wakeWaiters()
	{
		kDebug (1601);
		m_fileIsReady.wakeAll();
	}

	KFileItem RecursiveLister::getNextFile()
	{
		kDebug (1601);
		//first, check if there are any items available, if so take one.
		if (m_entryIndex < m_entries.size())
		{
			KFileItem item (m_entries.at(m_entryIndex));
			m_entryIndex++;
			return item;
		}

		//no items available, maybe we need to wait?
		if (!isFinished()) {
#ifndef KIO_IS_NOT_THREADSAFE
			m_waitLock.lock();
			kDebug (1601) << "will wait";
			m_fileIsReady.wait(&m_waitLock);
			kDebug (1601) << "through";
			m_waitLock.unlock();
#else
			kDebug (1601) << "Waiting until the whole job is finished";
			wait();
#endif
		}

		//ok, we have waited, let's see if there's something left for us
		if (m_entryIndex >= m_entries.size())
			return KFileItem();

		KFileItem item (m_entries.at(m_entryIndex));
		m_entryIndex++;
		return item;
	}

}
