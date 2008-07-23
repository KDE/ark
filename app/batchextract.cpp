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
#include <KPluginLoader>
#include <KPluginFactory>
#include <KMessageBox>
#include <KLocale>
#include "part/interface.h"
#include <KDebug>
#include <QTimer>

#include <QCoreApplication>


BatchExtract::BatchExtract(QWidget *parent)
	: KWidgetJobTracker(parent),
	m_part(NULL),
	m_arkInterface(NULL)
{
}

bool BatchExtract::loadPart()
{
	KPluginFactory *factory = KPluginLoader("libarkpart").factory();
	if(factory) {
		m_part = static_cast<KParts::ReadWritePart*>( factory->create<KParts::ReadWritePart>(this) );
	}
	if ( !factory || !m_part )
	{
		KMessageBox::error( NULL, i18n( "Unable to find Ark's KPart component, please check your installation." ) );
		return false;
	}
	m_part->setObjectName( "ArkPart" );

	m_arkInterface = qobject_cast<Interface*>(m_part);

	if (!m_arkInterface)
	{
		KMessageBox::error( NULL, i18n( "Unable to find Ark's KPart component, please check your installation." ) );
		return false;
	}
	m_part->widget()->setVisible(false);

	return true;
}

void BatchExtract::setInputFiles(QStringList files)
{
	m_inputFiles = files;
}

void BatchExtract::loadNextJob()
{
	if (m_inputFiles.isEmpty())
	{
		kDebug() << "OK DONE";
		QCoreApplication::quit();
		return;
	}

	if (m_showExtractDialog)
	{
		if (m_inputFiles.size() > 1)
		{
			m_arguments["extract"] = "batch";
			m_arguments["input"] = m_inputFiles;
		}
		else
		{
			m_arguments["input"] = m_inputFiles.first();
		}
		bool userAccepted = m_arkInterface->showExtractionDialog(m_arguments);

		//return unfinished signal
		if (!userAccepted)
			return;

		//we show the extractdialog only before the first job
		m_showExtractDialog = false;
	}

	kDebug() << "Starting listing...";
	m_part->openUrl(m_inputFiles.takeFirst());
	m_extractionTimerId = startTimer(150);

}

void BatchExtract::timerEvent ( class QTimerEvent * event )
{
	kDebug() << "waiting";
	if (!m_arkInterface->isBusy())
	{
		kDebug() << "Starting compression";
		startCompression();
		killTimer(m_extractionTimerId);
	}
}

void BatchExtract::startCompression()
{
	m_arkInterface->extract(m_arguments, this);
}

void BatchExtract::finished(KJob *job)
{
	KWidgetJobTracker::finished(job);
	kDebug() << "job finished...";
	loadNextJob();
}

void BatchExtract::registerJob (KJob *job)
{
	kDebug() << "Registering " << job;
	KWidgetJobTracker::registerJob(job);
	//setAutoDelete(job, false);
}

void BatchExtract::setDestinationDirectory(QString destination)
{
	m_arguments["destination"] = destination;
}

void BatchExtract::setInputFiles(QString file)
{
	setInputFiles(QStringList() << file);
}

void BatchExtract::setShowExtractDialog(bool value)
{
	m_showExtractDialog = value;
}

BatchExtract::~BatchExtract()
{
	delete m_part;
	m_part = 0;
}

#include <batchextract.moc>
