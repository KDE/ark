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
#include "kerfuffle/arch.h"

#include <QList>
#include <KLocale>

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
		}

		~ArchiveDirNode()
		{
			qDeleteAll( m_entries );
		}

		QList<ArchiveNode*>& entries() { return m_entries; }

		virtual bool isDir() const { return true; }

		ArchiveNode* find( const QString & name )
		{
			foreach( ArchiveNode *node, m_entries )
			{
				if ( node->name() == name )
				{
					return node;
				}
			}
			return 0;
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

ArchiveModel::ArchiveModel( Arch *archive, QObject *parent )
	: QAbstractItemModel( parent ), m_archive( archive ),
	  m_rootNode( new ArchiveDirNode( 0, ArchiveEntry() ) )
{
	m_archive->setParent( this );

	connect( m_archive, SIGNAL( newEntry( const ArchiveEntry& ) ),
	         this, SLOT( slotNewEntry( const ArchiveEntry& ) ) );
	m_archive->open();
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
				return node->name();
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
				return i18n( "Name" );
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
	return 1; // TODO: Completely bogus
}

ArchiveDirNode* ArchiveModel::parentFor( const ArchiveEntry& entry )
{
	QStringList pieces = entry[ FileName ].toString().split( '/', QString::SkipEmptyParts );
	pieces.removeLast();

	ArchiveDirNode *parent = m_rootNode;

	foreach( QString piece, pieces )
	{
		ArchiveNode *node = parent->find( piece );
		if ( !node )
		{
			ArchiveEntry e;
			e[ FileName ] = parent->entry()[ FileName ].toString() + '/' + piece;
			node = new ArchiveDirNode( parent, e );
		}
		if ( !node->isDir() )
		{
			ArchiveEntry e( node->entry() );
			int index = parent->entries().indexOf( node );
			delete node;
			node = new ArchiveDirNode( parent, e );
			parent->entries()[ index ] = node;
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

void ArchiveModel::slotNewEntry( const ArchiveEntry& entry )
{
	/// 1. Find Parent Node

	ArchiveDirNode *parent  = parentFor( entry ); // TODO: Don't make everyone child of the root, obey the hierarchy
	QModelIndex parentIndex = indexForNode( parent );


	/// 2. Create an ArchiveNode

	QString name = entry[ FileName ].toString().split( '/', QString::SkipEmptyParts ).last();
	ArchiveNode *node = parent->find( name );
	if ( node )
	{
		node->setEntry( entry );
	}
	else
	{
		beginInsertRows( parentIndex, m_rootNode->entries().count(), m_rootNode->entries().count() );

		if ( entry[ FileName ].toString().endsWith( '/' ) )
		{
			node = new ArchiveDirNode( parent, entry );
		}
		else
		{
			node = new ArchiveNode( parent, entry );
		}
		parent->entries().append( node );

		endInsertRows();
	}
}
