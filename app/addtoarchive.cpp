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
bool AddToArchive::addInput( const KUrl& url)
{
	m_inputs << url.path();
	return true;
}

bool AddToArchive::startAdding( void )
{
	kDebug( 1601 );
	Kerfuffle::Archive *archive = Kerfuffle::factory(m_filename);

	//TODO: needs error
	if (archive == NULL) {
		QCoreApplication::instance()->quit();
		return false;
	}

	if (!m_inputs.size()) {
		//TODO: needs error
		return false;
	}

	if (m_changeToFirstPath) {
		QString firstEntry = m_inputs.first();
		
		QDir stripDir = QFileInfo(firstEntry).dir();
		for (int i = 0; i < m_inputs.size(); ++i) {
			m_inputs[i] = stripDir.relativeFilePath(m_inputs.at(i));
		}

		QDir::setCurrent(stripDir.path());

	}

	Kerfuffle::AddJob *job = 
		archive->addFiles(m_inputs);


	KJobTrackerInterface *tracker = new KWidgetJobTracker(NULL);
	tracker->registerJob(job);

	connect(job, SIGNAL(finished(KJob*)),
			QCoreApplication::instance(), SLOT(quit()));

	job->start();

	return false;
}
