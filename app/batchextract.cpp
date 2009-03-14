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
#include <KMessageBox>
#include <KGlobal>

#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QApplication>
#include <QDesktopWidget>

BatchExtract::BatchExtract() 
	: autoSubfolders(false),
	destinationFolder(),
	m_preservePaths(true)

{
}

BatchExtract::~BatchExtract()
{
	kDebug(1601) << "Dying";
	KIO::getJobTracker()->unregisterJob(this);
}


void BatchExtract::addExtraction(Kerfuffle::Archive* archive,bool preservePaths, QString destinationFolder)
{
	kDebug( 1601 );

	QString autoDestination = destinationFolder;

	if (autoSubfolders) {
		if (!archive->isSingleFolderArchive()) {

			QString subfolder = archive->subfolderName();

			kDebug( 1601 ) << "Creating subfolder" << subfolder << "under" << destinationFolder;
			QDir dest(destinationFolder);
			dest.mkdir(subfolder);

			autoDestination = destinationFolder + '/' + subfolder;

		}
	}

	Kerfuffle::ExtractionOptions options;
	if (preservePaths)
		options["PreservePaths"] = true;

	Kerfuffle::ExtractJob *job = archive->copyFiles(
			QVariantList(), //extract all files
			autoDestination, //extract to folder
			options
			);

	connect(job, SIGNAL(userQuery(Query*)),
			this, SLOT(slotUserQuery(Query*)));

	kDebug( 1601 ) << QString("Registering job from archive %1, to %2, preservePaths %3").arg(archive->fileName()).arg(autoDestination).arg(preservePaths);


	addSubjob(job);
	fileNames[job] = qMakePair(archive->fileName(), destinationFolder);
	connect(job, SIGNAL(percent(KJob*, unsigned long)),
			this, SLOT(forwardProgress(KJob *, unsigned long)));

}

void BatchExtract::slotUserQuery(Query *query)
{
	query->execute();
}

void BatchExtract::setAutoSubfolder(bool value)
{
	autoSubfolders = value;
}

void BatchExtract::start()
{
	kDebug( 1601 );


	if (!subfolder.isEmpty()) {
		kDebug( 1601 ) << "Creating subfolder" << subfolder;
		QDir dest(destinationFolder);
		dest.mkpath(subfolder);
		destinationFolder += '/' + subfolder;
	}

	foreach (Kerfuffle::Archive *archive, inputs)
	{
		QString finalDestination;
		if (destinationFolder.isEmpty()) {
			finalDestination = QDir::currentPath();
		} else {
			finalDestination = destinationFolder;
		}

		addExtraction(archive, m_preservePaths, finalDestination);
	}
	KIO::getJobTracker()->registerJob(this);

	emit description(this,
			"Extracting file...",
			qMakePair(i18n("Source archive"), fileNames.value(subjobs().at(0)).first),
			qMakePair(i18n("Destination"), fileNames.value(subjobs().at(0)).second)
			);

	initialJobCount = subjobs().size();
	if (!initialJobCount) return;

	kDebug( 1601 ) << "Starting first job";

	subjobs().at(0)->start();
}

void BatchExtract::slotResult( KJob *job )
{
	kDebug( 1601 );
	KCompositeJob::slotResult(job);

	if ( job->error() )
	{
		kDebug(1601) << "There was en error, " << job->errorText();
		//TODO: why does this exit immediately
		KMessageBox::error( NULL, job->errorText());
		emitResult();
		return;
	}

	if (!subjobs().size())
	{
		kDebug(1601) << "Finished, emitting the result";
		emitResult();
	}
	else
	{
		kDebug(1601) << "Starting the next job";
		emit description(this,
				"Extracting file...",
				qMakePair(i18n("Source archive"), fileNames.value(subjobs().at(0)).first),
				qMakePair(i18n("Destination"), fileNames.value(subjobs().at(0)).second)
				);
		subjobs().at(0)->start();
	}
}

void BatchExtract::forwardProgress(KJob *job, unsigned long percent)
{
	Q_UNUSED(job);
	int jobPart = 100 / initialJobCount;
	setPercent( jobPart * (initialJobCount - subjobs().size()) + percent / initialJobCount );
}

bool BatchExtract::addInput( const KUrl& url )
{
	kDebug( 1601 );
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

void BatchExtract::setPreservePaths(bool value)
{
	m_preservePaths = value;
}
void BatchExtract::setSubfolder(QString subfolder)
{
	this->subfolder = subfolder;
}

bool BatchExtract::showExtractDialog()
{
	kDebug( 1601 );

	Kerfuffle::ExtractionDialog dialog(NULL);
	if (inputs.size() > 1) {
		dialog.batchModeOption();
	}

	if (destinationFolder.isEmpty())
		dialog.setCurrentUrl(QDir::currentPath());
	else
		dialog.setCurrentUrl(destinationFolder);

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


	if (dialog.extractToSubfolder()) {
		subfolder = dialog.subfolder();
	}

	autoSubfolders = dialog.autoSubfolders();
	m_preservePaths = dialog.preservePaths();

	return true;
}

#include <batchextract.moc>
