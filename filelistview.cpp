/*

  $Id$

  ark -- archiver for the KDE project

  Copyright (C)

  1999: Francois-Xavier Duranceau duranceau@kde.org
  1999-2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)
  2001: Corel Corporation (author: Michael Jarrett, michaelj@corel.com)
  2003: Georg Robbers <Georg.Robbers@urz.uni-hd.de>

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
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

// Qt includes
#include <qpainter.h>
#include <qwhatsthis.h>

// KDE includes
#include <klocale.h>
#include <kglobal.h>
#include <kdebug.h>
#include <kglobalsettings.h>

#include "filelistview.h"
#include "arkwidgetbase.h"

/////////////////////////////////////////////////////////////////////
// FileLVI implementation
/////////////////////////////////////////////////////////////////////

FileLVI::FileLVI(KListView* lv)
  : KListViewItem(lv)
{
}

/**
* Gets the filename associated with this FileLVI.
* This will return the filename of this entry, with any path information
* present.
* @return The filename associated with the FileLVI, stripped of any text
* 	used for formatting
* @warning Do NOT use text(0) for this purpose! This will get the filename
* 	with extra spaces for fileIndent, and is just plain shoddy code.
*/
QString FileLVI::fileName() const
{
	if(fileIndent)
		return text(0).mid(2);
	else
		return text(0);
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
            fileIndent = true;
        }
        else
        {
            QListViewItem::setText(column, text);
            fileIndent = false;
        }
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


FileListView::FileListView(ArkWidgetBase *baseArk, QWidget *parent,
			   const char* name)
	: KListView(parent, name), m_pParent(baseArk)
{
  sortColumn = 0;
  increasing = TRUE;
  QWhatsThis::add(this, i18n("This area is for displaying information about the files contained within an archive."));

  setMouseTracking(false);
  setSelectionModeExt(FileManager);
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


void FileListView::paintEmptyArea(QPainter *p, const QRect &rect)
{
	QListView::paintEmptyArea(p, rect);
	if(0 == childCount())
	{
		p->drawText(2, 16, i18n("No files in current archive"));
	}
}


void FileListView::setSorting(int column, bool inc)
{
	if(sortColumn == column)
	{
		increasing = !inc;
	}
	else{
		sortColumn = column;
		increasing = inc;
	}
	KListView::setSorting(sortColumn, increasing);
}

QStringList * FileListView::selectedFilenames() const
{
	QStringList *files = new QStringList;
	
	FileLVI * flvi = (FileLVI*)firstChild();

	while (flvi)
	{
		if( isSelected(flvi) )
			files->append(flvi->fileName());
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
	if(e->button()==QMouseEvent::LeftButton)
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
		QStringList *dragFiles = selectedFilenames();
		m_pParent->setDragInProgress(true);
		m_pParent->storeDragNames(*dragFiles);
		kdDebug(1601) << "Drag Starting..." << endl;
		m_pParent->prepareViewFiles(dragFiles);
		KListView::contentsMouseMoveEvent(e);
	}
}

#include "filelistview.moc"
