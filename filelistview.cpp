#include <assert.h>
#include <ctype.h>
#include <stdio.h>

// Qt includes
#include <qheader.h>
#include <qpixmap.h>
#include <qstrlist.h>

// KDE includes
#include <klocale.h>

#include "filelistview.h"
#include "filelistview.moc"


inline int max(int a, int b)
{
	return ((a) < (b) ? (b) : (a));
}

typedef const char* (*KeyFunc) (const char*);

/////////////////////////////////////////////////////////////////////
// FileListView implementation
/////////////////////////////////////////////////////////////////////

FileListView::FileListView(QWidget *parent, const char* name)
	: QListView(parent, name)
{
	sortColumn = 0;
	increasing = TRUE;
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
	QListView::setSorting(sortColumn, increasing);
}
