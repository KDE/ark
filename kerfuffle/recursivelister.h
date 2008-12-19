/*
 * ark -- archiver for the KDE project
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

#ifndef _RECURSIVELISTER_H_
#define _RECURSIVELISTER_H_

#include "kerfuffle_export.h"

#include <kjob.h>
#include <QThread>
#include <QWaitCondition>
#include <QMutex>
#include <kfileitem.h>
#include <kio/job.h>
#include <kio/jobclasses.h>

namespace Kerfuffle
{
	class KERFUFFLE_EXPORT RecursiveLister : public QThread
	{
		Q_OBJECT

		public:
			RecursiveLister(const QString& listDirectory);
			void run();
			KFileItem getNextFile();

		private slots:
			void slotEntry(KIO::Job *job, const KIO::UDSEntryList&);
			void wakeWaiters();

		private:
			QString m_listDirectory;
			QList<KFileItem> m_entries;
			int m_entryIndex;
			class KJob *m_listJob;
			QMutex m_waitLock;
			QWaitCondition m_fileIsReady;

	};
}

#endif /* ifndef _RECURSIVELISTER_H_ */
