/*

  ark -- archiver for the KDE project

  Copyright (C)

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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// Qt includes
#include <qpainter.h>
#include <qwhatsthis.h>

// KDE includes
#include <klocale.h>
#include <kglobal.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <kmimetype.h>
#include <kio/global.h>

#include "filelistview.h"
#include "arkwidget.h"
#include "archive.h"

ArkListViewItem::ArkListViewItem( const ArchiveEntry & entry, KListView * listView )
  : KListViewItem( listView ), m_entry( entry )
{
  KMimeType::Ptr mimeType = KMimeType::mimeType( m_entry.mimeType() );
  setText( nameColumn, m_entry.path() );
  setPixmap( nameColumn, mimeType->pixmap( KIcon::Small ) );
  setText( typeColumn, mimeType->comment() );
  setText( sizeColumn, KIO::convertSize( m_entry.size() ) );
  setText( compressedSizeColumn, KIO::convertSize( m_entry.compressedSize() ) );
  setText( ratioColumn, i18n( "%1%" ).arg( KGlobal::locale()->formatNumber( 100 * m_entry.compressionRatio(), 1 ) ) );
  setText( timeStampColumn, KGlobal::locale()->formatDateTime( m_entry.timeStamp() ) );
  setText( crcColumn, QString::number( m_entry.crc(), 16 ) );
}

QString ArkListViewItem::key(int column, bool ascending) const
{
  if ( column == 0 )
  {
    return m_entry.path();
  }

  return KListViewItem::key(column, ascending);
}

int ArkListViewItem::compare( QListViewItem * qItem, int column, bool ascending ) const
{
  if ( column == nameColumn or column == typeColumn )
  {
    return KListViewItem::compare( qItem, column, ascending );
  }

  ArkListViewItem * item = static_cast< ArkListViewItem * >( qItem );
  switch ( column )
  {
    case sizeColumn:
      {
        return ( size() < item->size() ? -1 :
               ( size() > item->size() ?  1 : 0 ) );
        break;
      }
    case compressedSizeColumn:
      {
        return ( compressedSize() < item->compressedSize() ? -1 :
               ( compressedSize() > item->compressedSize() ?  1 : 0 ) );
        break;
      }
    case ratioColumn:
      {
        return ( compressionRatio() < item->compressionRatio() ? -1 :
               ( compressionRatio() > item->compressionRatio() ?  1 : 0 ) );
        break;
      }
    case timeStampColumn:
      {
        return ( timeStamp() < item->timeStamp() ? -1 :
               ( timeStamp() > item->timeStamp() ?  1 : 0 ) );
        break;
      }
    default:
      return KListViewItem::compare( qItem, column, ascending );
  }
}

ArkView::ArkView( QWidget *parent, const char* name )
  : KListView( parent, name ), m_archive( 0 )
{
  setMultiSelection( true );
  setSelectionModeExt( FileManager );
  setDragEnabled( true );
  setItemsMovable( false );

  m_pressed = false;

  addColumn( i18n( "Name" ), 250 );
  addColumn( i18n( "Type" ), 220 );
  addColumn( i18n( "Size" ), 100 );
  addColumn( i18n( "Compressed Size" ), 100 );
  addColumn( i18n( "Compression Ratio" ), 50 );
  addColumn( i18n( "Timestamp" ), 150 );
  addColumn( i18n( "CRC" ), 100 );

  setColumnAlignment( sizeColumn, Qt::AlignRight );
  setColumnAlignment( ratioColumn, Qt::AlignRight );

  hideColumn( compressedSizeColumn );
  hideColumn( ratioColumn );
  hideColumn( crcColumn );

  setEnabled( false );
}

void ArkView::select( const QRegExp& regExp )
{
  // First, deselect everything that might be selected
  clearSelection();

  // Loops through all items, selecting the ones whose paths match the regular expression
  ArkListViewItem *item = static_cast< ArkListViewItem* >( firstChild() );
  while ( item )
  {
    if ( regExp.search( item->path() ) == 0 )
    {
      setSelected( item, true );
    }
    item = static_cast< ArkListViewItem* >( item->itemBelow() );
  }

}

QStringList ArkView::selectedFilenames() const
{
  QStringList files;

  ArkListViewItem * flvi = dynamic_cast< ArkListViewItem* >( firstChild() );

  while( flvi )
  {
    if( isSelected( flvi ) )
    {
      files.append(flvi->path());
    }

    flvi = dynamic_cast< ArkListViewItem* >( flvi->itemBelow() );
  }

  return files;
}

bool ArkView::isSelectionEmpty()
{
  ArkListViewItem * flvi = static_cast< ArkListViewItem* >( firstChild() );

  while (flvi)
  {
    if( flvi->isSelected() )
      return false;
    else
      flvi = static_cast< ArkListViewItem* >( flvi->itemBelow() );
  }

  return true;
}

void ArkView::contentsMousePressEvent(QMouseEvent *e)
{
  if( e->button() == QMouseEvent::LeftButton )
  {
    m_pressed = true;
    presspos = e->pos();
  }

  KListView::contentsMousePressEvent( e );
}

void ArkView::contentsMouseReleaseEvent(QMouseEvent *e)
{
  m_pressed = false;
  KListView::contentsMouseReleaseEvent(e);
}

void ArkView::contentsMouseMoveEvent(QMouseEvent *e)
{
  // If the left mouse button isn't pressed, then we shouldn't start a drag
  if( !m_pressed )
  {
    KListView::contentsMouseMoveEvent( e );
  }
  else if( ( presspos - e->pos() ).manhattanLength() > KGlobalSettings::dndEventDelay() )
  {
    m_pressed = false;	// Prevent triggering again

    if( isSelectionEmpty() )
    {
      return;
    }

    QStringList dragFiles = selectedFilenames();

    emit startDragRequest( dragFiles );

    KListView::contentsMouseMoveEvent( e );
  }
}

void ArkView::addItem( const ArchiveEntry& entry )
{
  ArkListViewItem *item;
  item = new ArkListViewItem( entry, this );
}

void ArkView::setArchive( Archive * archive )
{
  if ( m_archive )
  {
    disconnect( m_archive, SIGNAL( entryAdded(const ArchiveEntry&) ),
                this, SLOT( addItem(const ArchiveEntry&) ) );
  }
  
  m_archive = archive;

  if ( m_archive )
  {
    connect( m_archive, SIGNAL( entryAdded(const ArchiveEntry&) ),
             SLOT( addItem(const ArchiveEntry&) ) );
    setEnabled( true );
    return;
  }

  // If archive is null
  clear();
  setEnabled( false );
}

#include "filelistview.moc"
