//  -*-C++-*-           emacs magic for .h files
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
#ifndef FILELISTVIEW_H
#define FILELISTVIEW_H

#include <qlistview.h>
#include <klistview.h>

class QString;
class QStringList;
class QRect;
class QPainter;
class QColorGroup;
class QMouseEvent;
class QPoint;

class KListView;

class ArkWidgetBase;

class FileListView;

class FileLVI : public KListViewItem
{
public:
  FileLVI(KListView* lv) : KListViewItem(lv), parent(lv) {}

  QString getFileName() const;
  virtual QString key(int column, bool) const;
	virtual void setText(int column, const QString &text);

private:
  KListView *parent;
  bool fileIndent;	
};


class FileListView : public KListView
{
  Q_OBJECT
public:
  FileListView(ArkWidgetBase *baseArk, QWidget* parent = 0,
	       const char* name = 0);
  FileLVI *currentItem() {return ((FileLVI *) KListView::currentItem());}
  QStringList * selectedFilenames() const;
  uint count();
  bool isSelectionEmpty();

protected:
  void contentsMouseReleaseEvent(QMouseEvent *e);
  void contentsMousePressEvent(QMouseEvent *e);
  void contentsMouseMoveEvent(QMouseEvent *e);

  virtual void paintEmptyArea(QPainter * p, const QRect &rect);

private:
  int sortColumn;
  bool increasing;
  ArkWidgetBase *m_pParent;

  bool m_bPressed;
  QPoint presspos;  // this will save the click pos to correctly recognize drag events

  virtual void setSorting(int column, bool inc = TRUE);
};

#endif
