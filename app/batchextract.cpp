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
#include <KLocale>

#include <QFileInfo>
#include <QDir>

#include <QCoreApplication>

void BatchExtractJob::addExtraction(Kerfuffle::Archive* archive,bool preservePaths, QString destinationFolder)
{

	QString finalDestination;
	if (destinationFolder.isEmpty()) {
		finalDestination = QDir::currentPath();
	} else {
		finalDestination = destinationFolder;
	}

	Kerfuffle::ExtractJob *job = archive->copyFiles(
			QVariantList(), //extract all files
			finalDestination, //extract to current folder
			preservePaths //preserve paths
			);

	addSubjob(job);
	fileNames[job] = qMakePair(archive->fileName(), finalDestination);
	connect(job, SIGNAL(percent(KJob*, unsigned long)),
			this, SLOT(forwardProgress(KJob *, unsigned long)));

}

void BatchExtractJob::start()
{
	initialJobCount = subjobs().size();
	if (!initialJobCount) return;

	emit description(this,
			"Extracting file...",
			qMakePair(i18n("Source archive"), fileNames.value(subjobs().at(0)).first),
			qMakePair(i18n("Destination"), fileNames.value(subjobs().at(0)).second)
			);

	subjobs().at(0)->start();
}

void BatchExtractJob::slotResult( KJob *job )
{
	KCompositeJob::slotResult(job);
	if (!subjobs().size())
	{
		emitResult();
	}
	else
	{
		emit description(this,
				"Extracting file...",
				qMakePair(i18n("Source archive"), fileNames.value(subjobs().at(0)).first),
				qMakePair(i18n("Destination"), fileNames.value(subjobs().at(0)).second)
				);
		subjobs().at(0)->start();
	}
}

void BatchExtractJob::forwardProgress(KJob *job, unsigned long percent)
{
	Q_UNUSED(job);
	int jobPart = 100 / initialJobCount;
	setPercent( jobPart * (initialJobCount - subjobs().size()) + percent / initialJobCount );
}

BatchExtract::BatchExtract(QObject *) 
	: destinationFolder(QDir::currentPath()),
	autoSubfolders(true),
	m_preservePaths(true)

{

}

bool BatchExtract::addInput( const KUrl& url )
{
	Kerfuffle::Archive *archive = Kerfuffle::factory(url.path());
	if (archive == NULL) return false;

	inputs << archive;
	return true;
}

void BatchExtract::setDestinationFolder(QString folder)
{
	if (!folder.isEmpty())
		destinationFolder = folder;
}

void BatchExtract::setAutoSubfolder(bool value)
{
	autoSubfolders = value;
}

void BatchExtract::setPreservePaths(bool value)
{
	m_preservePaths = value;
}
void BatchExtract::setSubfolder(QString subfolder)
{
	this->subfolder = subfolder;
}

bool BatchExtract::startExtraction()
{
	BatchExtractJob *allJobs = new BatchExtractJob();
	tracker = new KWidgetJobTracker(NULL);

	foreach (Kerfuffle::Archive *archive, inputs)
	{
		allJobs->addExtraction(archive, m_preservePaths, destinationFolder);
	}
	tracker->registerJob(allJobs);

	connect(allJobs, SIGNAL(finished(KJob*)),
			QCoreApplication::instance(), SLOT(quit()));

	allJobs->start();
	return true;
}

bool BatchExtract::showExtractDialog()
{
	Kerfuffle::ExtractionDialog dialog(NULL);
	if (inputs.size() > 1) {
		dialog.batchModeOption();
	}

	dialog.setCurrentUrl(QDir::currentPath());
	dialog.setAutoSubfolder(autoSubfolders);
	dialog.setPreservePaths(m_preservePaths);

	if (subfolder.isEmpty() && inputs.size() == 1) {
		if (inputs.at(0)->isSingleFolderArchive()) {
			dialog.setSingleFolderArchive(true);
		}
		dialog.setSubfolder(inputs.at(0)->subfolderName());
	}
	else {
		dialog.setSubfolder(subfolder);
	}

	bool ret = dialog.exec();
	if (!ret) return false;

	setDestinationFolder(dialog.destinationDirectory().path());
	subfolder = dialog.subfolder();
	autoSubfolders = dialog.autoSubfolders();
	m_preservePaths = dialog.preservePaths();

	return true;
}

BatchExtract::~BatchExtract()
{

}

#include <batchextract.moc>
