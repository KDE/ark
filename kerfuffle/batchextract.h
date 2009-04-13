/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2008 Harald Hvaal <haraldhv (at@at) stud.ntnu.no>
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

#ifndef BATCHEXTRACT_H
#define BATCHEXTRACT_H

#include <KParts/MainWindow>
#include <KParts/ReadWritePart>
#include <KUrl>
#include <KDialog>
#include <QMap>
#include <QPair>
#include <kcompositejob.h>
#include "jobs.h"
#include "archive.h"
#include "queries.h"
#include "kerfuffle_export.h"

using Kerfuffle::Query;

class Interface;
class KJobTrackerInterface;

namespace Kerfuffle
{

	class KERFUFFLE_EXPORT BatchExtract : public KCompositeJob
	{
		Q_OBJECT

		public:
			BatchExtract();
			virtual ~BatchExtract();
			void addExtraction(Kerfuffle::Archive* archive, bool preservePaths = true, QString destinationFolder = QString());
			void start();
			void setAutoSubfolder(bool value);
			bool addInput( const KUrl& url );

		private slots:
			void forwardProgress(KJob *job, unsigned long percent);
			void slotResult( KJob *job );
			void slotUserQuery(Query *query);

		private:
			int initialJobCount;
			QMap<class KJob *, QPair<QString,QString> > fileNames;
			bool autoSubfolders;


			QList<Kerfuffle::Archive*> inputs;
			KJobTrackerInterface *tracker;
			QString destinationFolder;
			QString subfolder;
			bool m_preservePaths;

		public:
			bool showExtractDialog();
			void setDestinationFolder(QString folder);
			void setSubfolder(QString subfolder);
			void setPreservePaths(bool value);

		public slots:

	};

}


#endif // BATCHEXTRACT_H
