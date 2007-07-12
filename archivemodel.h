#ifndef ARCHIVEMODEL_H
#define ARCHIVEMODEL_H

#include <QAbstractItemModel>
#include "kerfuffle/arch.h"

class Arch;
class ArchiveDirNode;

class ArchiveModel: public QAbstractItemModel
{
	Q_OBJECT
	public:
		ArchiveModel( Arch *archive, QObject *parent = 0 );
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

	private slots:
		void slotNewEntry( const ArchiveEntry& entry );

	private:
		Arch *m_archive;
		ArchiveDirNode *m_rootNode;
};

#endif // ARCHIVEMODEL_H
