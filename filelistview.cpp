/*

  $Id$

  ark -- archiver for the KDE project

  Copyright (C)

  1999: Francois-Xavier Duranceau duranceau@kde.org
  1999-2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)

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
#include <qheader.h>
#include <qpixmap.h>
#include <qstringlist.h>

// KDE includes
#include <klocale.h>
#include <kdebug.h>

#include "filelistview.h"
#include "filelistview.moc"
#include "arkwidget.h"
#include "arch.h"

inline int max(int a, int b)
{
	return ((a) < (b) ? (b) : (a));
}

typedef const char* (*KeyFunc) (const char*);

/////////////////////////////////////////////////////////////////////
// FileLVI implementation
/////////////////////////////////////////////////////////////////////

QString FileLVI::getFileName()
{
    return text(0);
}

QString FileLVI::key(int column, bool ascending) const
{
    // puts numeric-type data into a field of 10 for correct sorting.
    static QString s;

    QString columnName = parent->columnText(column);
    if ( columnName == SIZE_STRING ||
	 columnName == PACKED_STRING )
      {
	s.sprintf("%.10ld", atol(text(column)));
	return s;
      }
    else if (columnName == RATIO_STRING)
      {
	char ratio[5];
	strcpy(ratio, text(column));
	ratio[strlen(ratio) - 1] = '\0';
	s.sprintf("%.10ld", atol(ratio));
	return s;
      }
    else return QListViewItem::key(column, ascending);
}


/////////////////////////////////////////////////////////////////////
// FileListView implementation
/////////////////////////////////////////////////////////////////////


FileListView::FileListView(QWidget *parent, const char* name)
  : KListView(parent, name), m_pParent((ArkWidget *)parent),
    m_bDropSourceIsSelf(false), m_pLastCurrentItem(0), m_pressed(false)
{
  sortColumn = 0;
  increasing = TRUE;
  setMouseTracking(false);
}

FileListView::~FileListView()
{
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
			files->append(flvi->text(0));
		flvi = (FileLVI*)flvi->itemBelow();
	}
	return files;
}

uint FileListView::count()
{
	uint c = 0;

	FileLVI * flvi = (FileLVI*)firstChild();

	while (flvi)
	{
		c++;
		flvi = (FileLVI*)flvi->itemBelow();
	}
	return c;
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

#if 0 /// this doesn't seem to work. :-(
void FileListView::contentsMouseMoveEvent(QMouseEvent *e)
{
  // takes care of the drag/extract

  if (!m_pressed)
    return;
  if (m_pParent->dragInProgress())
    return;
  struct stat statbuffer;
  const QPoint pPoint(viewport()->mapToGlobal(e->pos()));
  
  if (NULL == GetItemAt(pPoint))
    return;

  QListViewItem *pI;
  QStringList dragFiles;
  for (pI = firstChild(); pI != NULL; pI = pI->nextSibling())
  {
    if (isSelected(pI))
      {
	char name[BUFSIZ];
	strcpy(name, (const char *)pI->text(0));
	if (stat(name, &statbuffer) == -1)      // if it doesn't exist already
	  {   
	    dragFiles.append(name);
	  }
      }
  }
  m_pParent->setDragInProgress(true);
  m_pParent->storeNames(dragFiles);
  kdDebug(1601) << "Drag Starting..." << endl;
  m_pParent->unarchFile(&dragFiles);
}
#endif // end of stuff that doesn't quite work




void FileListView::contentsMousePressEvent(QMouseEvent *e)
{
  // does selection depending on these rules:
  // if it's the right mouse button, select the current and,
  // if it wasn't already selected, deselect the others.
  // Rationale: if it was already selected, this means someone probably wants
  // to do file operations on a bunch of selected files. But if it was
  // not selected, it's one its own.
  //
  // if it's the left button alone, select current and deselect others
  // if it's the left button with control, deselect all others
  // if it's the left button with shift, let mouseReleaseEvent look at
  //  the former current item and select all the ones between it and 
  //  the new current item.
  //
  QListViewItem *pItem = itemAt(e->pos());
  m_pLastCurrentItem = currentItem();
  setCurrentItem(pItem);

  if (e->button() == RightButton)
  {
    if (!isSelected(pItem))
      SelectCurrentOnly();
    KListView::contentsMousePressEvent(e);
  }
  else
  {
    bool bControl = (ControlButton == (e->state() & ControlButton));
    bool bShift = (ShiftButton == (e->state() & ShiftButton));
    if (e->button() == LeftButton)
    {
      m_pressed = true;
      if (!bControl && !bShift &&
	  !isSelected(pItem))       // so we can drag it
      {
	SelectCurrentOnly();
      }
      if (bControl)
      {
      	setSelected(pItem, !isSelected(pItem));
      }
    }
  }
}

void FileListView::contentsMouseReleaseEvent(QMouseEvent *e)
{
  //based on code from Corel File Manager/rightpanel.cpp
  //
  // Deals with shift range selection among other related tasks.
  //
  m_pressed = false;

  if (e->button() == LeftButton || e->button() == RightButton)
  {
    QListViewItem *pNewCurrentItem = itemAt(e->pos());

    //    const QPoint pPoint(viewport()->mapToGlobal(e->pos()));
    //		
    //    if (NULL == GetItemAt(pPoint))
    if (NULL == pNewCurrentItem)
    {
      setSelected(currentItem(), false);
      KListView::contentsMouseReleaseEvent(e);
      return;
    }

    if (e->button() == LeftButton)
    {
      //QListViewItem *pOldCurrentItem = currentItem();
      QListViewItem *pOldCurrentItem = m_pLastCurrentItem;
      setCurrentItem(pNewCurrentItem);
	
      if (ShiftButton == (e->state() & ShiftButton))
      {
	// First determine whether we should go up or down (from new to old)...
	QListViewItem *pI;
				
	bool bGoDown = false;
	
	for (pI = firstChild(); pI != NULL; pI = pI->nextSibling())
	{
	  if (pI == pNewCurrentItem)
	  {
	    bGoDown = true;
	    break;
	  }
	
	  if (pI == pOldCurrentItem)
	    break;
	}
	// Now build a list of items from the new to old in the correct order
	// ...Too bad Qt doesn't have prevSibling() :-(
	QList<QListViewItem> list;
	
	for (pI = bGoDown ? pNewCurrentItem : pOldCurrentItem;
	     pI != NULL; pI = pI->nextSibling())
	{
	  if (bGoDown)
	    list.append(pI);
	  else
	    list.insert(0, pI);
	
	  if (pI == (bGoDown ? pOldCurrentItem : pNewCurrentItem))
	    break;
	}
				
	// Now traverse the list we just built...
	
	QListIterator<QListViewItem> it(list);
	bool bCurrentFlag = true;
	
	for (; it.current() != NULL; ++it)
	{
	  pI = it.current();
	
	  if (ControlButton == (e->state() & ControlButton))
	  {
	    if (!isSelected(pI))
	      setSelected(pI, true);
	  }
	  else
	  {
	    bool bWasSelected = isSelected(pI);
	    setSelected(pI, bCurrentFlag);
	
	    if (bCurrentFlag && bWasSelected)
	      bCurrentFlag = false;
	  }
	}
      }
    }
    else
    {
      KListView::contentsMouseReleaseEvent(e);
    }
  }
  else
  {
    KListView::contentsMouseReleaseEvent(e);
  }
}


QListViewItem *FileListView::GetItemAt(const QPoint& p)
{
  QPoint pos = viewport()->mapFromGlobal(p);

  QListViewItem *pItem = itemAt(pos);
	
  if (NULL == pItem)// || iconView())
  {
    return pItem;
  }

  if (allColumnsShowFocus())
    return pItem;

  // otherwise make sure p is on the item
  int x = pos.x();

  if (x < header()->cellPos(header()->mapToActual(0)) ||
      x > header()->cellPos(header()->mapToActual(0)) + columnWidth(0))
  {
    return NULL;
  }

  QPainter painter;
  painter.begin(this);
  int w = painter.boundingRect(0, 0, columnWidth(0), 500, AlignLeft, pItem->text(0)).width();
  painter.end();

  const QPixmap *pPixmap = pItem->pixmap(0);
  int nPixWidth = 0;
  if (pPixmap != NULL)
  {
    nPixWidth = pPixmap->width();
  }
  if (x > header()->cellPos(header()->mapToActual(0)) + w + itemMargin()*2 +
      nPixWidth)
  {
    return NULL;
  }

  return pItem;
}


void FileListView::SelectCurrentOnly()
{
  QListViewItem *pI;

  for (pI = firstChild(); pI != NULL; pI = pI->nextSibling())
  {
    if (isSelected(pI) && pI != currentItem())
      setSelected(pI, false);
  }

  if (!isSelected(currentItem()))
    setSelected(currentItem(), true);
}
