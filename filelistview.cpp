/*

  ark -- archiver for the KDE project

  Copyright (C) 2005 Henrique Pinto <henrique.pinto@kdemail.net>
  Copyright (C) 2003 Georg Robbers <Georg.Robbers@urz.uni-hd.de>
  Copyright (C) 2001 Corel Corporation (author: Michael Jarrett, <michaelj@corel.com>)
  Copyright (C) 1999-2000 Corel Corporation (author: Emily Ezust, <emilye@corel.com>)
  Copyright (C) 1999 Francois-Xavier Duranceau <duranceau@kde.org>

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

#include "filelistview.h"

// Qt includes
#include <QPainter>
#include <QMouseEvent>
#include <QHeaderView>
#include <QTreeWidgetItemIterator>
#include <QFont>
// KDE includes
#include <KLocale>
#include <KIconLoader>
#include <KGlobal>
#include <KDebug>
#include <KGlobalSettings>
#include <KMimeType>

FileListView::FileListView( QWidget *parent )
	: QTreeWidget( parent )
{
	setSelectionMode( QAbstractItemView::ExtendedSelection );
	setHeaders();
	/*
	setShowSortIndicator( true );
	setResizeMode( Q3ListView::AllColumns );
	m_pressed = false;*/
}

QStringList FileListView::selectedFilenames()
{
	QStringList files;

	QTreeWidgetItemIterator it( this, QTreeWidgetItemIterator::Selected );
	while ( *it )
	{
		ArchiveEntry e = m_entryMap[ *it ];
		files << e[ FileName ].toString();
		++it;
	}

	// TODO: this needs to be ported
#warning "Reimplement this..."
#if 0
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
#endif

	return files;
}

QStringList FileListView::fileNames()
{
	QStringList files;

	QTreeWidgetItemIterator it( this );
	while ( *it )
	{
		Q_ASSERT( m_entryMap.contains( *it ) );
		ArchiveEntry entry = m_entryMap[ *it ];
		files += entry[ FileName ].toString();
		++it;
	}

	return files;
}

bool FileListView::isSelectionEmpty()
{
	return selectedItems().size() == 0;
}

ArchiveEntry FileListView::item( const QString& filename )
{
	QTreeWidgetItemIterator it( this );
	while ( *it )
	{
		Q_ASSERT( m_entryMap.contains( *it ) );
		ArchiveEntry entry = m_entryMap[ *it ];
		if ( entry[ FileName ].toString() == filename )
		{
			return entry;
		}
		++it;
	}

	return ArchiveEntry();
}

void FileListView::addItem( const ArchiveEntry & entry )
{
	QTreeWidgetItem *item = new QTreeWidgetItem( this );

	m_entryMap[ item ] = entry;
	item->setText( 0, entry[ FileName ].toString() );
	if ( entry.contains( Size ) )
		item->setText( 1, KIO::convertSize( entry[ Size ].toULongLong() ) );

	if ( entry.contains( Link ) )
	{
		item->setText( 5, entry[ Link ].toString() );
		QFont font;
		font.setItalic( true );
		item->setFont( 0, font );
	}

	item->setText( 2, entry[ Owner ].toString() );
	item->setText( 3, entry[ Group ].toString() );
	item->setText( 4, KGlobal::locale()->formatDateTime( entry[ Timestamp ].toDateTime(), KLocale::FancyShortDate, true ) );

	KMimeType::Ptr mimeType = KMimeType::findByPath( entry[ FileName ].toString(), 0, true );
	item->setIcon( 0, KIconLoader::global()->loadMimeTypeIcon(mimeType->iconName(), K3Icon::Small ) );
#warning "Port me"
#if 0
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
	flvi->setPixmap( 0, KIconLoader::global()->loadMimeTypeIcon(mimeType->iconName(), K3Icon::Small ) );
#endif
}

void FileListView::setHeaders()
{
	QStringList l;
	l << "Filename";
	l << "Size";
	l << "Owner";
	l << "Group";
	l << "Timestamp";
	l << "Link";

	setHeaderLabels( l );
	header()->setResizeMode( QHeaderView::ResizeToContents );
}

int FileListView::totalFiles()
{
	int numFiles = 0;

	QTreeWidgetItemIterator it( this, QTreeWidgetItemIterator::NoChildren );
	while ( *it )
	{
		++numFiles;
		++it;
	}

	return numFiles;
}

int FileListView::selectedFilesCount()
{
	return selectedItems().size();
}

KIO::filesize_t FileListView::totalSize()
{
	KIO::filesize_t size = 0;

	QTreeWidgetItemIterator it( this, QTreeWidgetItemIterator::NoChildren );
	while ( *it )
	{
		Q_ASSERT( m_entryMap.contains( *it ) );
		ArchiveEntry entry = m_entryMap[ *it ];
		size += entry[ Size ].toULongLong();
		++it;
	}

	return size;
}

KIO::filesize_t FileListView::selectedSize()
{
	KIO::filesize_t size = 0;

	QTreeWidgetItemIterator it( this, QTreeWidgetItemIterator::Selected );
	while ( *it )
	{
		Q_ASSERT( m_entryMap.contains( *it ) );
		ArchiveEntry entry = m_entryMap[ *it ];
		size += entry[ Size ].toULongLong();
		++it;
	}

	return size;
}

QTreeWidgetItem* FileListView::findParent( const QString& fullname )
{
	return 0;
#warning "Reimplement"
	/*QString name = fullname;

	if ( name.endsWith( "/" ) )
		name = name.left( name.length() -1 );
	if ( name.startsWith( "/" ) )
		name = name.mid( 1 );
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
	return static_cast< FileLVI* >( item );*/
}

QStringList FileListView::childrenOf( QTreeWidgetItem* parent )
{
	Q_ASSERT( parent );
	QStringList children;

#warning Reimplement
	/*FileLVI *item = static_cast<FileLVI*>( parent->firstChild() );

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
	}*/

	return children;
}

ArchiveEntry FileListView::currentEntry() const
{
	return m_entryMap[ currentItem() ];
}

#include "filelistview.moc"
// kate: space-indent off; tab-width 4;
