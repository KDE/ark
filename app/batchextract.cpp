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

void BatchExtraction::addExtraction(Kerfuffle::ExtractJob *job)
{
	addSubjob(job);
}

void BatchExtraction::start()
{
	foreach (KJob *job, subjobs())
	{
		job->exec();
	}
	emitResult();
}

BatchExtract::BatchExtract(QWidget *parent)
	: KDialog(parent)
{

}

void BatchExtract::addInput( const KUrl& url )
{
	inputs << url.path();
}

bool BatchExtract::performExtraction()
{
	BatchExtraction *allJobs = new BatchExtraction();
	tracker = new KWidgetJobTracker(NULL);

	foreach (QString filename, inputs)
	{

		Kerfuffle::Archive *archive = Kerfuffle::factory(filename);
		if (archive == NULL) continue;

		Kerfuffle::ExtractJob *job = archive->copyFiles(
				QVariantList(), //extract all files
				".", //extract to current folder
				true //preserve paths
				);

		allJobs->addExtraction(job);
	}
	tracker->registerJob(allJobs);
	allJobs->start();
	return true;
}

void BatchExtract::showExtractDialog()
{
	Kerfuffle::ExtractionDialog dialog(this);
	dialog.exec();
}

BatchExtract::~BatchExtract()
{

}

#include <batchextract.moc>
