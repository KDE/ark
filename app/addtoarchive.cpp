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

#include "addtoarchive.h"
#include "kerfuffle/adddialog.h"
#include <kdebug.h>
#include <QCoreApplication>
#include <kwidgetjobtracker.h>
#include <QFileInfo>
#include <QDir>

AddToArchive::AddToArchive(QObject *parent)
	: QObject(parent), m_changeToFirstPath(false)
{

}

AddToArchive::~AddToArchive()
{

}

bool AddToArchive::showAddDialog( void )
{
	Kerfuffle::AddDialog dialog(
			m_inputs,
			KUrl(m_firstPath),
			"",
			NULL,
			NULL);
	bool ret = dialog.exec();

	if (ret) {
		kDebug( 1601 ) << "Dialog succeeded, returned url " <<
			dialog.selectedUrl();
		setFilename(dialog.selectedUrl());
	}
	return ret;
}

bool AddToArchive::addInput( const KUrl& url)
{
	m_inputs << url.path();

	if (m_firstPath.isEmpty()) {
		QString firstEntry = url.path();

		//we chop off "/" at the end. if not QFileInfo will be confused about
		//whether its a directory or not.
		if (firstEntry.right(1) == "/")
			firstEntry.chop(1);

		QFileInfo firstFI = QFileInfo(firstEntry);
		
		m_firstPath = firstFI.dir().absolutePath();

	}

	return true;
}

bool AddToArchive::startAdding( void )
{
	kDebug( 1601 );

	Kerfuffle::CompressionOptions options;

	if (!m_inputs.size()) {
		//TODO: needs error
		return false;
	}

	Kerfuffle::Archive *archive;
	if (!m_filename.isEmpty()) {
		 archive = Kerfuffle::factory(m_filename);
		 kDebug( 1601 ) << "Set filename to " + m_filename;
	}
	else {
		
		if (m_autoFilenameSuffix.isEmpty()) {
			//TODO: needs error
			return false;
		}

		if (m_firstPath.isEmpty()) {
			//TODO: needs error
			return false;
		}

		QString base;
		QFileInfo fi(m_inputs.first());

		base = fi.absoluteFilePath();

		if (base.right(1) == "/") {
			base.chop(1);
		}

		QString finalName = base + "." + m_autoFilenameSuffix;

		kDebug( 1601 ) << "Autoset filename to " + finalName;
		archive = Kerfuffle::factory(finalName);
		
	}


	//TODO: needs error
	if (archive == NULL) {
		QCoreApplication::instance()->quit();
		return false;
	}

	if (m_changeToFirstPath) {
		if (m_firstPath.isEmpty()) {
			//TODO: needs error
			return false;
		}

		QDir stripDir = QDir(m_firstPath);

		for (int i = 0; i < m_inputs.size(); ++i) {
			m_inputs[i] = stripDir.absoluteFilePath(m_inputs.at(i));
		}

		options["GlobalWorkDir"] = stripDir.path();
		kDebug( 1601 ) << "Setting GlobalWorkDir to " << stripDir.path();

	}

	Kerfuffle::AddJob *job = 
		archive->addFiles(m_inputs, options);


	KJobTrackerInterface *tracker = new KWidgetJobTracker(NULL);
	tracker->registerJob(job);

	connect(job, SIGNAL(finished(KJob*)),
			QCoreApplication::instance(), SLOT(quit()));

	job->start();

	return false;
}
