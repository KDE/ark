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

#include "filelistview.h"
#include "arkwidget.h"

/////////////////////////////////////////////////////////////////////
// FileLVI implementation
/////////////////////////////////////////////////////////////////////

FileLVI::FileLVI(KListView* lv)
  : KListViewItem(lv)
{
}

/**
 * Returns the size of the file, or 0 if no size is defined.
 */
long FileLVI::fileSize() const
{
  return m_fileSize;
}

long FileLVI::packedFileSize() const
{
  return m_packedFileSize;
}

double FileLVI::ratio() const
{
  return m_ratio;
}

QDateTime FileLVI::timeStamp() const
{
  return m_timeStamp;
}

QString FileLVI::key(int column, bool ascending) const
{
    if ( 0 == column )
        return fileName();
    return QListViewItem::key(column, ascending);
}

int FileLVI::compare( QListViewItem * i, int col, bool ascending ) const
{
    if ( col == 0 )
        return KListViewItem::compare( i, col, ascending );

    FileLVI * item = static_cast< FileLVI * >( i );
    columnName colName = ( static_cast< FileListView * > ( listView() ) )->nameOfColumn( col );
    switch ( colName )
    {
        case sizeCol:
                        {
                            return ( m_fileSize < item->fileSize() ? -1 :
                                    ( m_fileSize > item->fileSize() ? 1 : 0 ) );
                            break;
                        }
        case ratioStrCol:
                        {
                            return ( m_ratio < item->ratio() ? -1 :
                                    ( m_ratio > item->ratio() ? 1 : 0 ) );
                            break;
                        }

        case packedStrCol:
                        {
                            return ( m_packedFileSize < item->packedFileSize() ? -1 :
                                    ( m_packedFileSize > item->packedFileSize() ? 1 : 0 ) );
                            break;
                        }
        case timeStampStrCol:
                        {
                            return ( m_timeStamp < item->timeStamp() ? -1 :
                                    ( m_timeStamp > item->timeStamp() ? 1 : 0 ) );
                            break;
                        }
        default:
                        return KListViewItem::compare( i, col, ascending );
    }
}

void FileLVI::setText(int column, const QString &text)
{
    columnName colName = ( static_cast< FileListView * > ( listView() ) )->nameOfColumn( column );
    if ( column == 0 )
    {
        if (text.findRev('/', -2) != -1)
        {
            QListViewItem::setText(0, QString("  ") + text);
        }
        else
        {
            QListViewItem::setText(column, text);
        }
        m_entryName = text;
    }
    else if ( colName == sizeCol )
    {
        m_fileSize = text.toLong();
        QListViewItem::setText(column, KGlobal::locale()->formatNumber(m_fileSize, 0));
    }
    else if ( colName == packedStrCol )
    {
        m_packedFileSize = text.toLong();
        QListViewItem::setText(column, KGlobal::locale()->formatNumber(m_packedFileSize, 0));
    }
    else if ( colName == ratioStrCol )
    {
        int l = text.length() - 1;
        if ( l>0 && text[l] == '%' )
            m_ratio = text.left(l).toDouble();
        else
            m_ratio = text.toDouble();
        QListViewItem::setText(column, i18n("Packed Ratio", "%1 %").arg(KGlobal::locale()->formatNumber(m_ratio, 1)));
    }
    else if ( colName == timeStampStrCol )
    {
        m_timeStamp = QDateTime::fromString(text, ISODate);
        QListViewItem::setText(column, KGlobal::locale()->formatDateTime(m_timeStamp));
    }
    else
        QListViewItem::setText(column, text);
}

/////////////////////////////////////////////////////////////////////
// FileListView implementation
/////////////////////////////////////////////////////////////////////


FileListView::FileListView(ArkWidget *baseArk, QWidget *parent,
			   const char* name)
	: KListView(parent, name), m_pParent(baseArk)
{
  QWhatsThis::add(this, i18n("This area is for displaying information about the files contained within an archive."));

  setMultiSelection( true );
  setSelectionModeExt( FileManager );
  setDragEnabled( true );
  setItemsMovable( false );

  m_bPressed = false;
}

int FileListView::addColumn ( const QString & label, int width )
{
    int index = KListView::addColumn( label, width );
    if ( label == SIZE_STRING )
    {
        colMap[ index ] = sizeCol;
    }
    else if ( label == PACKED_STRING )
    {
        colMap[ index ] = packedStrCol;
    }
    else if ( label == RATIO_STRING )
    {
        colMap[ index ] = ratioStrCol;
    }
    else if ( label == TIMESTAMP_STRING )
    {
        colMap[ index ] = timeStampStrCol;
    }
    else
    {
        colMap[ index ] = otherCol;
    }
    return index;
}

void FileListView::removeColumn( int index )
{
    unsigned int i = index;
    for ( ; i < colMap.count() - 2; i++ )
        colMap.replace( i, colMap[ i + 1 ] );
    colMap.remove( colMap[ colMap.count() - 1 ] );
    KListView::removeColumn( index );
}

columnName FileListView::nameOfColumn( int index )
{
    return colMap[ index ];
}

QStringList FileListView::selectedFilenames() const
{
	QStringList files;

	FileLVI * flvi = (FileLVI*)firstChild();

	while (flvi)
	{
		if( isSelected(flvi) )
			files.append(flvi->fileName());
		flvi = (FileLVI*)flvi->itemBelow();
	}
	return files;
}

uint FileListView::count()
{
	return childCount();
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
	if( e->button()==QMouseEvent::LeftButton )
	{
		m_bPressed = true;
		presspos = e->pos();
	}

	KListView::contentsMousePressEvent(e);
}

void
FileListView::contentsMouseReleaseEvent(QMouseEvent *e)
{
	m_bPressed = false;
	KListView::contentsMouseReleaseEvent(e);
}

void
FileListView::contentsMouseMoveEvent(QMouseEvent *e)
{
	if(!m_bPressed)
	{
		KListView::contentsMouseMoveEvent(e);
	}
	else if( ( presspos - e->pos() ).manhattanLength() > KGlobalSettings::dndEventDelay() )
	{
		m_bPressed = false;	// Prevent triggering again
		if(isSelectionEmpty())
		{
			return;
		}
		QStringList dragFiles = selectedFilenames();
		emit startDragRequest( dragFiles );
		KListView::contentsMouseMoveEvent(e);
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
	FileLVI *flvi = new FileLVI( this );

	int i = 0;

	for (QStringList::ConstIterator it = entries.begin(); it != entries.end(); ++it)
	{
		flvi->setText(i, *it);
		++i;
	}

	KMimeType::Ptr mimeType = KMimeType::findByPath( entries.first(), 0, true );
	flvi->setPixmap( 0, mimeType->pixmap( KIcon::Small ) );
}

void FileListView::selectAll()
{
	QListView::selectAll( true );
}

void FileListView::unselectAll()
{
	QListView::selectAll( false );
}

void FileListView::setHeaders( const ColumnList& columns )
{
	clearHeaders();

	for ( ColumnList::const_iterator it = columns.constBegin();
	      it != columns.constEnd();
	      ++it )
	{
		QPair< QString, Qt::AlignmentFlags > pair = *it;
		int colnum = addColumn( pair.first );
		setColumnAlignment( colnum, pair.second );
	}
}

void FileListView::clearHeaders()
{
	while ( columns() > 0 )
	{
		removeColumn( 0 );
	}
}

#include "filelistview.moc"
