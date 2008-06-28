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
#include "archivemodel.h"
#include "kerfuffle/archive.h"
#include "kerfuffle/jobs.h"

#include <QList>
#include <QPixmap>

#include <KDebug>
#include <KLocale>
#include <KMimeType>
#include <KIconLoader>
#include <KIO/NetAccess>

class ArchiveDirNode;

class ArchiveNode
{
	public:
		ArchiveNode( ArchiveDirNode *parent, const ArchiveEntry & entry )
			: m_parent( parent ), m_row( -1 )
		{
			setEntry( entry );
		}

		virtual ~ArchiveNode() {}

		ArchiveEntry entry() const { return m_entry; }
		void setEntry( const ArchiveEntry & entry )
		{
			m_entry = entry;
			QStringList pieces = entry[ FileName ].toString().split( '/', QString::SkipEmptyParts );
			m_name = pieces.isEmpty()? QString() : pieces.last();
		}

		ArchiveDirNode *parent() const { return m_parent; }

		int row();
		QString name() const { return m_name; }

		virtual bool isDir() const { return false; }

		QPixmap icon()
		{
			if ( m_icon.isNull() )
			{
				KMimeType::Ptr mimeType = KMimeType::findByPath( m_entry[ FileName ].toString(), 0, true );
				m_icon = KIconLoader::global()->loadMimeTypeIcon( mimeType->iconName(), KIconLoader::Small );
			}
			return m_icon;
		}

	protected:
		QPixmap         m_icon;

	private:
		ArchiveEntry    m_entry;
		ArchiveDirNode *m_parent;
		QString         m_name;
		int             m_row;
};

class ArchiveDirNode: public ArchiveNode
{
	public:
		ArchiveDirNode( ArchiveDirNode *parent, const ArchiveEntry & entry )
			: ArchiveNode( parent, entry )
		{
			m_icon = KIconLoader::global()->loadMimeTypeIcon( KMimeType::mimeType( "inode/directory" )->iconName(), KIconLoader::Small );
		}

		~ArchiveDirNode()
		{
			clear();
		}

		QList<ArchiveNode*>& entries() { return m_entries; }

		virtual bool isDir() const { return true; }

		ArchiveNode* find( const QString & name )
		{
			foreach( ArchiveNode *node, m_entries )
			{
				if ( node && ( node->name() == name ) )
				{
					return node;
				}
			}
			return 0;
		}

		ArchiveNode* findByPath( const QString & path )
		{
			QStringList pieces = path.split( '/' );
			if ( pieces.isEmpty() )
			{
				return 0;
			}

			ArchiveNode *next = find( pieces[ 0 ] );

			if ( pieces.count() == 1 )
			{
				return next;
			}
			if ( next && next->isDir() )
			{
				//pieces.removeAt(0);
				return static_cast<ArchiveDirNode*>( next )->findByPath( pieces.join( "/" ) );
			}
			return 0;
		}

		void clear()
		{
			qDeleteAll( m_entries );
			m_entries.clear();
		}

	private:
		QList<ArchiveNode*> m_entries;
};

int ArchiveNode::row()
{
	if ( m_row != -1 ) return m_row;

	if ( parent() )
	{
		m_row = parent()->entries().indexOf( const_cast<ArchiveNode*>( this ) );
		return m_row;
	}
	return 0;
}

ArchiveModel::ArchiveModel( QObject *parent )
	: QAbstractItemModel( parent ), m_archive( 0 ),
	  m_rootNode( new ArchiveDirNode( 0, ArchiveEntry() ) ),
	  m_jobTracker(0)
{
}

ArchiveModel::~ArchiveModel()
{
	delete m_archive;
	m_archive = 0;

	delete m_rootNode;
	m_rootNode = 0;
}

QVariant ArchiveModel::data( const QModelIndex &index, int role ) const
{
	if ( index.isValid() )
	{
		ArchiveNode *node = static_cast<ArchiveNode*>( index.internalPointer() );
		switch ( role )
		{
			case Qt::DisplayRole:
				if ( index.column() == 0 )
				{
					return node->name();
				}
				else
				{
					if ( node->isDir() || node->entry().contains( Link ) )
					{
						return QVariant();
					}
					else
					{
						return KIO::convertSize( node->entry()[ Size ].toULongLong() );
					}
				}
			case Qt::DecorationRole:
				if ( index.column() == 0 )
				{
					return node->icon();
				}
				return QVariant();
			default:
				return QVariant();
		}
	}
	return QVariant();
}

Qt::ItemFlags ArchiveModel::flags( const QModelIndex &index ) const
{
	if ( index.isValid() )
	{
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	}

	return 0;
}

QVariant ArchiveModel::headerData( int section, Qt::Orientation, int role ) const
{
	if ( role == Qt::DisplayRole )
	{
		switch ( section )
		{
			case 0:
				return i18nc( "Name of a file inside an archive", "Name" );
			case 1:
				return i18nc( "Uncompressed size of a file inside an archive", "Size" );
		}
	}
	return QVariant();
}

QModelIndex ArchiveModel::index( int row, int column, const QModelIndex &parent ) const
{
	if ( hasIndex( row, column, parent ) )
	{
		ArchiveDirNode *parentNode = parent.isValid()? static_cast<ArchiveDirNode*>( parent.internalPointer() ) : m_rootNode;

		Q_ASSERT( parentNode->isDir() );

		ArchiveNode *item = parentNode->entries().value( row, 0 );
		if ( item )
		{
			return createIndex( row, column, item );
		}
	}

	return QModelIndex();
}

QModelIndex ArchiveModel::parent( const QModelIndex &index ) const
{
	if ( index.isValid() )
	{
		ArchiveNode *item = static_cast<ArchiveNode*>( index.internalPointer() );
		Q_ASSERT( item );
		if ( item->parent() && ( item->parent() != m_rootNode ) )
		{
			return createIndex( item->parent()->row(), 0, item->parent() );
		}
	}
	return QModelIndex();
}

ArchiveEntry ArchiveModel::entryForIndex( const QModelIndex &index )
{
	if ( index.isValid() )
	{
		ArchiveNode *item = static_cast<ArchiveNode*>( index.internalPointer() );
		Q_ASSERT( item );
		return item->entry();
	}
	return ArchiveEntry();
}

int ArchiveModel::childCount( const QModelIndex &index )
{
	if ( index.isValid() )
	{
		ArchiveNode *item = static_cast<ArchiveNode*>( index.internalPointer() );
		Q_ASSERT( item );
		if ( item->isDir() )
		{
			return static_cast<ArchiveDirNode*>( item )->entries().count();
		}
		return 0;
	}
	return -1;
}

int ArchiveModel::rowCount( const QModelIndex &parent ) const
{
	if ( parent.column() <= 0 )
	{
		ArchiveNode *parentNode = parent.isValid()? static_cast<ArchiveNode*>( parent.internalPointer() ) : m_rootNode;

		if ( parentNode && parentNode->isDir() )
		{
			return static_cast<ArchiveDirNode*>( parentNode )->entries().count();
		}
	}
	return 0;
}

int ArchiveModel::columnCount( const QModelIndex &parent ) const
{
	if ( parent.isValid() )
	{
		return static_cast<ArchiveNode*>( parent.internalPointer() )->entry().size();
	}
	return 2; // TODO: Completely bogus
}

ArchiveDirNode* ArchiveModel::parentFor( const ArchiveEntry& entry )
{
	QStringList pieces = entry[ FileName ].toString().split( '/', QString::SkipEmptyParts );
	pieces.removeLast();

	ArchiveDirNode *parent = m_rootNode;

	foreach( const QString &piece, pieces )
	{
		ArchiveNode *node = parent->find( piece );
		if ( !node )
		{
			ArchiveEntry e;
			e[ FileName ] = parent->entry()[ FileName ].toString() + '/' + piece;
			e[ IsDirectory ] = true;
			node = new ArchiveDirNode( parent, e );
			insertNode( node );
		}
		if ( !node->isDir() )
		{
			ArchiveEntry e( node->entry() );
			node = new ArchiveDirNode( parent, e );
			//Maybe we have both a file and a directory of the same name
			// We avoid removing previous entries unless necessary
			insertNode( node );
		}
		parent = static_cast<ArchiveDirNode*>( node );
	}

	return parent;
}
QModelIndex ArchiveModel::indexForNode( ArchiveNode *node )
{
	Q_ASSERT( node );
	if ( node != m_rootNode )
	{
		Q_ASSERT( node->parent() );
		Q_ASSERT( node->parent()->isDir() );
		return createIndex( node->row(), 0, node );
	}
	return QModelIndex();
}

void ArchiveModel::slotEntryRemoved( const QString & path )
{
	// TODO: Do something
	ArchiveNode *entry = m_rootNode->findByPath( path );
	if ( entry )
	{
		ArchiveDirNode *parent = entry->parent();
		QModelIndex index = indexForNode( entry );

		beginRemoveRows( indexForNode( parent ), entry->row(), entry->row() );

		delete parent->entries()[ entry->row() ];
		parent->entries()[ entry->row() ] = 0;

		endRemoveRows();
	}
}

void ArchiveModel::slotNewEntry( const ArchiveEntry& entry )
{
	kDebug (1601) << entry; 
	/// 1. Skip already created nodes
	if (m_rootNode){
		ArchiveNode *existing = m_rootNode->findByPath( entry[ FileName ].toString() );
		if ( existing ) {
			kDebug (1601) << "Skipping entry creation for" << entry[FileName].toString(); 
			return;
		}
	}

	/// 2. Find Parent Node, creating missing ArchiveDirNodes in the process
	ArchiveDirNode *parent  = parentFor( entry ); 
	
	/// 3. Create an ArchiveNode
	QString name = entry[ FileName ].toString().split( '/', QString::SkipEmptyParts ).last();
	ArchiveNode *node = parent->find( name );
	if ( node )
	{
		node->setEntry( entry );
	}
	else
	{
		if ( entry[ FileName ].toString().endsWith( '/' ) || ( entry.contains( IsDirectory ) && entry[ IsDirectory ].toBool() ) )
		{
			node = new ArchiveDirNode( parent, entry );
		}
		else
		{
			node = new ArchiveNode( parent, entry );
		}
		insertNode( node );
	}
}

void ArchiveModel::insertNode( ArchiveNode *node )
{
	Q_ASSERT(node);
	ArchiveDirNode *parent = node->parent();
	Q_ASSERT(parent);
	beginInsertRows( indexForNode( parent ), parent->entries().count(), parent->entries().count() );
	parent->entries().append( node );
	endInsertRows();
}

void ArchiveModel::setArchive( Kerfuffle::Archive *archive )
{
	delete m_archive;
	m_archive = archive;
	m_rootNode->clear();
	if ( m_archive )
	{
		Kerfuffle::ListJob *job = m_archive->list(); // TODO: call "open" or "create"?

		connect( job, SIGNAL( newEntry( const ArchiveEntry& ) ),
			 this, SLOT( slotNewEntry( const ArchiveEntry& ) ) );

		connect( job, SIGNAL( result( KJob * ) ),
		         this, SIGNAL( loadingFinished() ) );

		if ( m_jobTracker )
		{
			m_jobTracker->registerJob( job );
		}

		emit loadingStarted();
		job->start();
	}
	reset();
}

ExtractJob* ArchiveModel::extractFile( const QVariant& fileName, const QString & destinationDir, bool preservePaths )
{
	QList<QVariant> files;
	files << fileName;
	return extractFiles( files, destinationDir, preservePaths );
}

ExtractJob* ArchiveModel::extractFiles( const QList<QVariant>& files, const QString & destinationDir, bool preservePaths )
{
	Q_ASSERT( m_archive );
	return m_archive->copyFiles( files, destinationDir, preservePaths );
}

AddJob* ArchiveModel::addFiles( const QStringList & paths )
{
	Q_ASSERT( m_archive );

    if ( !m_archive->isReadOnly())
    {
        AddJob *job = m_archive->addFiles( paths );
        m_jobTracker->registerJob( job );
        connect( job, SIGNAL( newEntry( const ArchiveEntry& ) ),
            this, SLOT( slotNewEntry( const ArchiveEntry& ) ) );
        return job;
    }
    return 0;
}

DeleteJob* ArchiveModel::deleteFiles( const QList<QVariant> & files )
{
	Q_ASSERT( m_archive );
	if ( !m_archive->isReadOnly() )
	{
		DeleteJob *job = m_archive->deleteFiles( files );
		m_jobTracker->registerJob( job );
		connect( job, SIGNAL( entryRemoved( const QString & ) ),
		         this, SLOT( slotEntryRemoved( const QString & ) ) );
		return job;
	}
	return 0;
}
