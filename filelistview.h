//  -*-C++-*-           emacs magic for .h files
/*

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
#ifndef FILELISTVIEW_H
#define FILELISTVIEW_H

#include <klistview.h>
#include <qstringlist.h>
#include <qwidget.h>

class FileListView;

class FileLVI : public QListViewItem
{
public:
	FileLVI(KListView* lv) : QListViewItem(lv), parent(lv) {}
	QString getFileName();

	virtual QString key(int column, bool) const;
private:
    KListView *parent;
	
};


class FileListView : public KListView
{
  Q_OBJECT
public:
	FileListView(QWidget* parent = 0, const char* name = 0);
	~FileListView();
	FileLVI *currentItem() {return ((FileLVI *) KListView::currentItem());}
	QStringList * selectedFilenames() const;
	uint count();
	bool isSelectionEmpty();
private:
	int sortColumn;
	bool increasing;
	
	virtual void setSorting(int column, bool inc = TRUE);
};

#endif
