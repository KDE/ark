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

#include "batchextract.h"

#include "kerfuffle/extractiondialog.h"

#include <kwidgetjobtracker.h>
#include <KDebug>
#include <QFileInfo>
#include <QDir>

#include <QCoreApplication>

void BatchExtraction::addExtraction(Kerfuffle::ExtractJob *job)
{
	addSubjob(job);
	connect(job, SIGNAL(percent(KJob*, unsigned long)),
			this, SLOT(forwardProgress(KJob *, unsigned long)));

}

void BatchExtraction::start()
{
	initialJobCount = subjobs().size();
	if (!initialJobCount) return;

	subjobs().at(0)->start();
}

void BatchExtraction::slotResult( KJob *job )
{
	KCompositeJob::slotResult(job);
	if (!subjobs().size())
	{
		emitResult();
	}
	else
	{
		subjobs().at(0)->start();
	}
}

void BatchExtraction::forwardProgress(KJob *job, unsigned long percent)
{
	int jobPart = 100 / initialJobCount;
	setPercent( jobPart * (initialJobCount - subjobs().size()) + percent / initialJobCount );
}

BatchExtract::BatchExtract(QObject *parent)
{

}

void BatchExtract::addInput( const KUrl& url )
{
	inputs << url.path();
}

void BatchExtract::setDestinationFolder(QString folder)
{
	destinationFolder = folder;
}

bool BatchExtract::performExtraction()
{
	BatchExtraction *allJobs = new BatchExtraction();
	tracker = new KWidgetJobTracker(NULL);

	foreach (QString filename, inputs)
	{

		Kerfuffle::Archive *archive = Kerfuffle::factory(filename);
		if (archive == NULL) continue;

		QString finalDestination;
		if (destinationFolder.isEmpty()) {
			finalDestination = QFileInfo(filename).dir().absolutePath();
		} else {
			finalDestination = destinationFolder;
		}

		Kerfuffle::ExtractJob *job = archive->copyFiles(
				QVariantList(), //extract all files
				finalDestination, //extract to current folder
				true //preserve paths
				);

		allJobs->addExtraction(job);
	}
	tracker->registerJob(allJobs);

	connect(allJobs, SIGNAL(finished(KJob*)),
			QCoreApplication::instance(), SLOT(quit()));

	allJobs->start();
	return true;
}

void BatchExtract::showExtractDialog()
{
	Kerfuffle::ExtractionDialog dialog(NULL);
	dialog.exec();
}

BatchExtract::~BatchExtract()
{

}

#include <batchextract.moc>
