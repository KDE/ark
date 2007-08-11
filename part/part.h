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
#ifndef PART_H
#define PART_H

#include <KParts/Part>
#include <KParts/StatusBarExtension>
#include <QModelIndex>
#include "interface.h"

class ArchiveModel;
class InfoPanel;

class QTreeView;
class QAction;

class KAboutData;
class KTempDir;
class KJob;
class KAbstractWidgetJobTracker;

class Part: public KParts::ReadWritePart, public Interface
{
	Q_OBJECT
	Q_INTERFACES( Interface )
	public:
		Part( QWidget *parentWidget, QObject *parent, const QStringList & );
		~Part();
		static KAboutData* createAboutData();

		virtual bool openFile();
		virtual bool saveFile();

		QStringList supportedMimeTypes() const;
		QStringList supportedWriteMimeTypes() const;

		bool isBusy() const { return m_busy; }

	private slots:
		void slotLoadingStarted();
		void slotLoadingFinished();
		void slotPreview();
		void slotPreview( const QModelIndex & index );
		void slotPreviewExtracted( KJob* );
		void slotError( const QString& errorMessage, const QString& details );
		void slotExtractFiles();
		void slotExtractionDone( KJob* );
		void slotAddFiles();
		void slotAddDir();
		void slotAddFilesDone( KJob* );
		void slotDeleteFiles();
		void updateActions();
		void selectionChanged();
		void adjustColumns( const QModelIndex & topleft, const QModelIndex& bottomRight );
		void createJobTracker();

	signals:
		void busy();
		void ready();

	private:
		void setupView();
		void setupActions();
		bool isPreviewable( const QModelIndex & index );
		QList<QVariant> selectedFiles();

		ArchiveModel         *m_model;
		QTreeView            *m_view;
		QAction              *m_previewAction;
		QAction              *m_extractFilesAction;
		QAction              *m_addFilesAction;
		QAction              *m_addDirAction;
		QAction              *m_deleteFilesAction;
		InfoPanel            *m_infoPanel;
		KTempDir             *m_previewDir;
		bool                  m_busy;

		KAbstractWidgetJobTracker  *m_jobTracker;
		KParts::StatusBarExtension *m_statusBarExtension;
};

#endif // PART_H
