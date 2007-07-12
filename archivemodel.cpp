#include "archivemodel.h"
#include "kerfuffle/arch.h"

#include <QList>
#include <KLocale>

class ArchiveDirNode;

class ArchiveNode
{
	public:
		ArchiveNode( ArchiveDirNode *parent, const ArchiveEntry & entry )
			: m_entry( entry ), m_parent( parent )
		{
		}

		virtual ~ArchiveNode() {}

		ArchiveEntry entry() const { return m_entry; }
		ArchiveDirNode *parent() const { return m_parent; }

		int row() const;

		virtual bool isDir() const { return false; }


	private:
		ArchiveEntry    m_entry;
		ArchiveDirNode *m_parent;
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

	private:
		QList<ArchiveNode*> m_entries;
};

int ArchiveNode::row() const
{
	if ( parent() )
	{
		return parent()->entries().indexOf( const_cast<ArchiveNode*>( this ) );
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
				return node->entry()[ FileName ];
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
		if ( item->parent() && ( item->parent() != m_rootNode ) )
		{
			return createIndex( item->parent()->row(), 0, item->parent() );
		}
	}
	return QModelIndex();
}

int ArchiveModel::rowCount( const QModelIndex &parent ) const
{
	kDebug( 1601 ) << k_funcinfo << "parent.column() is " << parent.column() << endl;
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

void ArchiveModel::slotNewEntry( const ArchiveEntry& entry )
{
	/// 1. Find Parent Node

	ArchiveDirNode *parent  = m_rootNode; // TODO: Don't make everyone child of the root, obey the hierarchy
	QModelIndex parentIndex = QModelIndex();

	/// 2. Create an ArchiveNode

	beginInsertRows( parentIndex, m_rootNode->entries().count(), m_rootNode->entries().count() );

	ArchiveNode *node = new ArchiveNode( parent, entry );
	parent->entries().append( node );

	kDebug( 1601 ) << "New entry for: " << entry[ FileName ] << endl;

	endInsertRows();
}
