/*

  ark -- archiver for the KDE project

  Copyright (C)

  2005: Henrique Pinto <henrique.pinto@kdemail.net>
  2003: Georg Robbers <Georg.Robbers@urz.uni-hd.de>
  2001: Corel Corporation (author: Michael Jarrett, michaelj@corel.com)
  1999-2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)
  1999: Francois-Xavier Duranceau duranceau@kde.org

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

// Qt includes
#include <qpainter.h>
#include <q3whatsthis.h>
#include <QMouseEvent>
#include <Q3Header>

// KDE includes
#include <klocale.h>
#include <kglobal.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <kmimetype.h>

#include "filelistview.h"
#include "archive.h"

/////////////////////////////////////////////////////////////////////
// FileLVI implementation
/////////////////////////////////////////////////////////////////////

FileLVI::FileLVI( K3ListView* lv )
	: K3ListViewItem( lv ), m_fileSize( 0 ), m_packedFileSize( 0 ),
	  m_ratio( 0 ), m_timeStamp( QDateTime() ), m_entryName( QString() )
{

}

FileLVI::FileLVI( K3ListViewItem* lvi )
	: K3ListViewItem( lvi ), m_fileSize( 0 ), m_packedFileSize( 0 ),
	  m_ratio( 0 ), m_timeStamp( QDateTime() ), m_entryName( QString() )
{
}

QString FileLVI::key( int column, bool ascending ) const
{
	if ( column == 0 )
		return fileName();
	else
		return Q3ListViewItem::key( column, ascending );
}

int FileLVI::compare( Q3ListViewItem * i, int column, bool ascending ) const
{
	if ( column == 0 )
		return K3ListViewItem::compare( i, column, ascending );

	FileLVI * item = static_cast< FileLVI * >( i );
	columnName colName = ( static_cast< FileListView * > ( listView() ) )->nameOfColumn( column );
	switch ( colName )
	{
		case sizeCol:
		{
			return ( m_fileSize < item->fileSize() ? -1 :
			           ( m_fileSize > item->fileSize() ? 1 : 0 )
			       );
			break;
		}

		case ratioStrCol:
		{
			return ( m_ratio < item->ratio() ? -1 :
			           ( m_ratio > item->ratio() ? 1 : 0 )
			       );
			break;
		}

		case packedStrCol:
		{
			return ( m_packedFileSize < item->packedFileSize() ? -1 :
			           ( m_packedFileSize > item->packedFileSize() ? 1 : 0 )
			       );
			break;
		}

		case timeStampStrCol:
		{
			return ( m_timeStamp < item->timeStamp() ? -1 :
			           ( m_timeStamp > item->timeStamp() ? 1 : 0 )
			       );
			break;
		}

		default:
			return K3ListViewItem::compare( i, column, ascending );
	}
}

void FileLVI::setText( int column, const QString &text )
{
	columnName colName = ( static_cast< FileListView * > ( listView() ) )->nameOfColumn( column );
	if ( column == 0 )
	{
		QString name = text;
		if ( name.endsWith( "/" ) )
			name = name.left( name.length() - 1 );
		int pos = name.lastIndexOf( '/' );
		if ( pos != -1 )
			name = name.right( name.length() - pos - 1 );
		Q3ListViewItem::setText( column, name );
		m_entryName = text;
	}
	else if ( colName == sizeCol )
	{
		m_fileSize = text.toLong();
		Q3ListViewItem::setText( column, KIO::convertSize( m_fileSize ) );
	}
	else if ( colName == packedStrCol )
	{
		m_packedFileSize = text.toLong();
		Q3ListViewItem::setText( column, KIO::convertSize( m_packedFileSize ) );
	}
	else if ( colName == ratioStrCol )
	{
		int l = text.length() - 1;
		if ( l>0 && text[l] == '%' )
			m_ratio = text.left(l).toDouble();
		else
			m_ratio = text.toDouble();
		Q3ListViewItem::setText( column, i18nc( "Packed Ratio", "%1 %" ,
		                                 KGlobal::locale()->formatNumber( m_ratio, 1 ) )
		                      );
	}
	else if ( colName == timeStampStrCol )
	{
		m_timeStamp = QDateTime::fromString( text, Qt::ISODate );
		Q3ListViewItem::setText( column, KGlobal::locale()->formatDateTime( m_timeStamp ) );
	}
	else
		Q3ListViewItem::setText(column, text);
}

static FileLVI* folderLVI( K3ListViewItem *parent, const QString& name )
{
	FileLVI *folder = new FileLVI( parent );
	folder->setText( 0, name );
	folder->setPixmap( 0, KMimeType::mimeType( "inode/directory" )->pixmap( K3Icon::Small ) );
	return folder;
}

static FileLVI* folderLVI( K3ListView *parent, const QString& name )
{
	FileLVI *folder = new FileLVI( parent );
	folder->setText( 0, name );
	folder->setPixmap( 0, KMimeType::mimeType( "inode/directory" )->pixmap( K3Icon::Small ) );
	return folder;
}

/////////////////////////////////////////////////////////////////////
// FileListView implementation
/////////////////////////////////////////////////////////////////////


FileListView::FileListView( QWidget *parent )
	: K3ListView( parent )
{
	setMultiSelection( true );
	setSelectionModeExt( FileManager );
	setDragEnabled( true );
	setItemsMovable( false );
	setRootIsDecorated( true );
	setShowSortIndicator( true );
	setResizeMode( Q3ListView::AllColumns );
	header()->hide(); // Don't show the header until there is something to be shown in it
	m_pressed = false;
}

int FileListView::addColumn ( const QString & label, int width )
{
	int index = K3ListView::addColumn( label, width );
	if ( label == SIZE_COLUMN.first )
	{
		m_columnMap[ index ] = sizeCol;
	}
	else if ( label == PACKED_COLUMN.first )
	{
		m_columnMap[ index ] = packedStrCol;
	}
	else if ( label == RATIO_COLUMN.first )
	{
		m_columnMap[ index ] = ratioStrCol;
	}
	else if ( label == TIMESTAMP_COLUMN.first )
	{
		m_columnMap[ index ] = timeStampStrCol;
	}
	else
	{
		m_columnMap[ index ] = otherCol;
	}
	return index;
}

void FileListView::removeColumn( int index )
{
	for ( int i = index; i < m_columnMap.count() - 2; i++ )
	{
		m_columnMap.insert( i, m_columnMap[ i + 1 ] );
	}

	m_columnMap.remove( m_columnMap[ m_columnMap.count() - 1 ] );
	K3ListView::removeColumn( index );
}

columnName FileListView::nameOfColumn( int index )
{
	return m_columnMap[ index ];
}

QStringList FileListView::selectedFilenames()
{
	QStringList files;

	FileLVI *item = static_cast<FileLVI*>( firstChild() );

	while ( item )
	{
		if ( item->isSelected() )
		{
			// If the item has children, add each child instead of the item
			if ( item->childCount() > 0 )
			{
				files += childrenOf( item );

				/* If we got here, then the logic for "going to the next item"
				 * is a bit different: as we already dealt with all the children,
				 * the "next item" is the next sibling of the current item, not
				 * its first child. If the current item has no siblings, then
				 * the next item is the next sibling of its parent, and so on.
				 */
				FileLVI *nitem = static_cast<FileLVI*>( item->nextSibling() );
				while ( !nitem and item->parent() )
				{
					item = static_cast<FileLVI*>( item->parent() );
					nitem = static_cast<FileLVI*>( item->parent()->nextSibling() );
				}
				item = nitem;
				continue;
			}
			else
			{
				// If the item has no children, just add it to the list
				files += item->fileName();
			}
		}
		// Go to the next item
		item = static_cast<FileLVI*>( item->itemBelow() );
	}

	return files;
}

QStringList FileListView::fileNames()
{
	QStringList files;

	Q3ListViewItemIterator it( this );
	while ( it.current() )
	{
		FileLVI *item = static_cast<FileLVI*>( it.current() );
		files += item->fileName();
		++it;
	}

	return files;
}

bool FileListView::isSelectionEmpty()
{
	FileLVI * flvi = (FileLVI*)firstChild();

	while (flvi)
	{
		if( flvi->isSelected() )
			return false;
		else
		flvi = (FileLVI*)flvi->itemBelow();
	}
	return true;
}

void
FileListView::contentsMousePressEvent(QMouseEvent *e)
{
	if( e->button() == Qt::LeftButton )
	{
		m_pressed = true;
		m_presspos = e->pos();
	}

	K3ListView::contentsMousePressEvent(e);
}

void
FileListView::contentsMouseReleaseEvent(QMouseEvent *e)
{
	m_pressed = false;
	K3ListView::contentsMouseReleaseEvent(e);
}

void
FileListView::contentsMouseMoveEvent(QMouseEvent *e)
{
	if(!m_pressed)
	{
		K3ListView::contentsMouseMoveEvent(e);
	}
	else if( ( m_presspos - e->pos() ).manhattanLength() > KGlobalSettings::dndEventDelay() )
	{
		m_pressed = false;	// Prevent triggering again
		if(isSelectionEmpty())
		{
			return;
		}
		QStringList dragFiles = selectedFilenames();
		emit startDragRequest( dragFiles );
		K3ListView::contentsMouseMoveEvent(e);
	}
}

FileLVI*
FileListView::item(const QString& filename) const
{
	FileLVI * flvi = (FileLVI*) firstChild();

	while (flvi)
	{
		QString curFilename = flvi->fileName();
		if (curFilename == filename)
			return flvi;
		flvi = (FileLVI*) flvi->nextSibling();
	}

	return 0;
}

void FileListView::addItem( const QStringList & entries )
{
	FileLVI *flvi, *parent = findParent( entries[0] );
	if ( parent )
		flvi = new FileLVI( parent );
	else
		flvi = new FileLVI( this );


	int i = 0;

	for (QStringList::ConstIterator it = entries.begin(); it != entries.end(); ++it)
	{
		flvi->setText(i, *it);
		++i;
	}

	KMimeType::Ptr mimeType = KMimeType::findByPath( entries.first(), 0, true );
	flvi->setPixmap( 0, mimeType->pixmap( K3Icon::Small ) );
}

void FileListView::selectAll()
{
	Q3ListView::selectAll( true );
}

void FileListView::unselectAll()
{
	Q3ListView::selectAll( false );
}

void FileListView::setHeaders( const ColumnList& columns )
{
	clearHeaders();

	for ( ColumnList::const_iterator it = columns.constBegin();
	      it != columns.constEnd();
	      ++it )
	{
		QPair< QString, Qt::AlignmentFlag > pair = *it;
		int colnum = addColumn( pair.first );
		setColumnAlignment( colnum, pair.second );
	}

	header()->show();
}

void FileListView::clearHeaders()
{
	header()->hide();
	while ( columns() > 0 )
	{
		removeColumn( 0 );
	}
}

int FileListView::totalFiles()
{
	int numFiles = 0;

	Q3ListViewItemIterator it( this );
	while ( it.current() )
	{
		if ( it.current()->childCount() == 0 )
			++numFiles;
		++it;
	}

	return numFiles;
}

int FileListView::selectedFilesCount()
{
	int numFiles = 0;

	Q3ListViewItemIterator it( this, Q3ListViewItemIterator::Selected );
	while ( it.current() )
	{
		++numFiles;
		++it;
	}

	return numFiles;
}

KIO::filesize_t FileListView::totalSize()
{
	KIO::filesize_t size = 0;

	Q3ListViewItemIterator it(this);
	while ( it.current() )
	{
		FileLVI *item = static_cast<FileLVI*>( it.current() );
		size += item->fileSize();
		++it;
	}

	return size;
}

KIO::filesize_t FileListView::selectedSize()
{
	KIO::filesize_t size = 0;

	Q3ListViewItemIterator it( this, Q3ListViewItemIterator::Selected );
	while ( it.current() )
	{
		FileLVI *item = static_cast<FileLVI*>( it.current() );
		size += item->fileSize();
		++it;
	}

	return size;
}

FileLVI* FileListView::findParent( const QString& fullname )
{
	QString name = fullname;

	if ( name.endsWith( "/" ) )
		name = name.left( name.length() -1 );
	// Checks if this entry needs a parent
	if ( !name.contains( '/' ) )
		return static_cast< FileLVI* >( 0 );

	// Get a list of ancestors
	QString parentFullname = name.left( name.lastIndexOf( '/' ) );
	QStringList ancestorList;
	if (parentFullname.isEmpty())
		ancestorList = QStringList();
	else
		ancestorList = parentFullname.split( '/' );

	// Checks if the listview contains the first item in the list of ancestors
	Q3ListViewItem *item = firstChild();
	while ( item )
	{
		if ( item->text( 0 ) == ancestorList[0] )
			break;
		item = item->nextSibling();
	}

	// If the list view does not contain the item, create it
	if ( !item )
	{
		item = folderLVI( this, ancestorList[0] );
	}

	// We've already dealt with the first item, remove it
	ancestorList.pop_front();

	while ( ancestorList.count() > 0 )
	{
		QString name = ancestorList[0];

		FileLVI *parent = static_cast< FileLVI*>( item );
		item = parent->firstChild();
		while ( item )
		{
			if ( item->text(0) == name )
				break;
			item = item->nextSibling();
		}

		if ( !item )
		{
			item = folderLVI( parent, name );
		}

		ancestorList.pop_front();
	}

	item->setOpen( true );
	return static_cast< FileLVI* >( item );
}

QStringList FileListView::childrenOf( FileLVI* parent )
{
	Q_ASSERT( parent );
	QStringList children;

	FileLVI *item = static_cast<FileLVI*>( parent->firstChild() );

	while ( item )
	{
		if ( item->childCount() == 0 )
		{
			children += item->fileName();
		}
		else
		{
			children += childrenOf( item );
		}
		item = static_cast<FileLVI*>( item->nextSibling() );
	}

	return children;
}

#include "filelistview.moc"
// kate: space-indent off; tab-width 4;
