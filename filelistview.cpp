/*

  $Id$

  ark -- archiver for the KDE project

  Copyright (C)

  1999: Francois-Xavier Duranceau duranceau@kde.org
  1999-2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)
  2001: Corel Corporation (author: Michael Jarrett, michaelj@corel.com)

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
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

// Qt includes
#include <qpainter.h>
#include <qwhatsthis.h>

// KDE includes
#include <klocale.h>
#include <kdebug.h>

#include "filelistview.h"
#include "arch.h"
#include "arkwidgetbase.h"

inline int max(int a, int b)
{
	return ((a) < (b) ? (b) : (a));
}

typedef const char* (*KeyFunc) (const char*);

/////////////////////////////////////////////////////////////////////
// FileLVI implementation
/////////////////////////////////////////////////////////////////////

/**
* Gets the filename associated with this FileLVI.
* This will return the filename of this entry, with any path information
* present.
* @return The filename associated with the FileLVI, stripped of any text
* 	used for formatting
* @warning Do NOT use text(0) for this purpose! This will get the filename
* 	with extra spaces for fileIndent, and is just plain shoddy code.
*/
QString FileLVI::getFileName() const
{
	if(fileIndent)
		return text(0).mid(2);
	else
		return text(0);
}

QString FileLVI::key(int column, bool ascending) const
{
    // puts numeric-type data into a field of 10 for correct sorting.
    QString s;

    QString columnName = parent->columnText(column);
    if ( columnName == SIZE_STRING ||
	 columnName == PACKED_STRING )
      {
	s.sprintf("%.10ld", text(column).toInt());
	return s;
      }
    else if (columnName == RATIO_STRING)
      {
	s.sprintf("%.10ld", text(column).toInt());
	return s;
      }
		else if(0 == column)
			return getFileName();
    else return QListViewItem::key(column, ascending);
}

void FileLVI::setText(int column, const QString &text)
{
	if(0 == column && -1 != text.findRev('/', -2))
	{
		QListViewItem::setText(0, QString("  ") + text);
		fileIndent = true;
	}
	else
	{
		if(0 == column)
			fileIndent = false;
		QListViewItem::setText(column, text);
	}
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
			files->append(flvi->getFileName());
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

void FileListView::contentsMousePressEvent(QMouseEvent *e)
{
	m_bPressed = true;
	KListView::contentsMousePressEvent(e);
}

void FileListView::contentsMouseReleaseEvent(QMouseEvent *e)
{
	m_bPressed = false;
	KListView::contentsMouseReleaseEvent(e);
}

void FileListView::contentsMouseMoveEvent(QMouseEvent *e)
{
	if(!m_bPressed)
	{
		KListView::contentsMouseMoveEvent(e);
	}
	else
	{
		m_bPressed = false;	// Prevent triggering again
		if(isSelectionEmpty())
			return;
		QStringList *dragFiles = selectedFilenames();
		m_pParent->setDragInProgress(true);
		m_pParent->storeDragNames(*dragFiles);
		kdDebug(1601) << "Drag Starting..." << endl;
		m_pParent->prepareViewFiles(dragFiles);
	}
}

#include "filelistview.moc"
