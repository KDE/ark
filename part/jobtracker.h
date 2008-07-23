/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
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
#ifndef JOBTRACKER_H
#define JOBTRACKER_H

#include <QFrame>
#include <kabstractwidgetjobtracker.h>
#include "ui_jobtracker.h"

class JobTrackerWidget: public QFrame, public Ui::JobTrackerWidget
{
	Q_OBJECT
	public:
	JobTrackerWidget( QWidget *parent = 0 );
};

class JobTracker: public KAbstractWidgetJobTracker
{
	Q_OBJECT
	public:
		JobTracker( QWidget *parent = 0 );

		QWidget *widget(KJob *job) { return m_ui; }
		virtual void registerJob( KJob *job );
		virtual void unregisterJob( KJob *job );

		KJob *currentJob() const { return m_currentJob; }

	protected slots:
		virtual void finished( KJob *job );
		virtual void description( KJob *job, const QString &title, const QPair< QString, QString > &f1, const QPair< QString, QString > &f2 );
		virtual void infoMessage( KJob *job, const QString &plain, const QString &rich );
		virtual void warning( KJob *job, const QString &plain, const QString &rich );

		virtual void percent (KJob *job, unsigned long 	percent	);

	private slots:
		void timeOut();

	private:
		JobTrackerWidget *m_ui;
		KJob             *m_currentJob;
};

#endif // JOBTRACKER_H
