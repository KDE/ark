#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
// Qt includes
#include <qheader.h>
#include <qpixmap.h>
#include <qstringlist.h>

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
    if ( (columnName == QString("Size")) ||
	 (columnName == QString("Length")))       
    {
	s.sprintf("%.10ld", atol(text(column)));
	return s;
    }
    else return QListViewItem::key(column, ascending);
}


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

