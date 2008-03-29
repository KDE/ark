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
#ifndef ARCHIVEMODEL_H
#define ARCHIVEMODEL_H

#include <QAbstractItemModel>
#include <kjobtrackerinterface.h>
#include "kerfuffle/archive.h"

class ArchiveNode;
class ArchiveDirNode;

using namespace Kerfuffle;

class ArchiveModel: public QAbstractItemModel
{
	Q_OBJECT
	public:
		ArchiveModel( QObject *parent = 0 );
		~ArchiveModel();

		QVariant data( const QModelIndex &index, int role ) const;
		Qt::ItemFlags flags( const QModelIndex &index ) const;
		QVariant headerData( int section, Qt::Orientation orientation,
		                     int role = Qt::DisplayRole ) const;
		QModelIndex index( int row, int column,
		                   const QModelIndex &parent = QModelIndex() ) const;
		QModelIndex parent( const QModelIndex &index ) const;
		int rowCount( const QModelIndex &parent = QModelIndex() ) const;
		int columnCount( const QModelIndex &parent = QModelIndex() ) const;

		void setArchive( Kerfuffle::Archive *archive );
		Kerfuffle::Archive *archive() const { return m_archive; }

		ArchiveEntry entryForIndex( const QModelIndex &index );
		int childCount( const QModelIndex &index );

		ExtractJob* extractFile( const QVariant& fileName, const QString & destinationDir, bool preservePaths = false );
		ExtractJob* extractFiles( const QList<QVariant>& files, const QString & destinationDir, bool preservePaths = false );

		AddJob* addFiles( const QStringList & paths );
		DeleteJob* deleteFiles( const QList<QVariant> & files );

		void setJobTracker( KJobTrackerInterface *tracker ) { m_jobTracker = tracker; }

	signals:
		void loadingStarted();
		void loadingFinished();
		void extractionFinished( bool success );
		void error( const QString& error, const QString& details );

	private slots:
		void slotNewEntry( const ArchiveEntry& entry );
		void slotEntryRemoved( const QString & path );

	private:
		ArchiveDirNode* parentFor( const ArchiveEntry& entry );
		QModelIndex indexForNode( ArchiveNode *node );
		/**
		 * Insert the node @p node into the model, ensuring all views are notified
		 * of the change.
		 */
		void insertNode( ArchiveNode *node );

		Kerfuffle::Archive *m_archive;
		ArchiveDirNode *m_rootNode;
		KJobTrackerInterface *m_jobTracker;
};

#endif // ARCHIVEMODEL_H
