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

static const char * intKey(const char* text);
static const char * timeKey(const char* text);

static const char*
intKey(const char* text)
{
	int val;
	sscanf(text, "%d", &val);
	static char key[16];
	sprintf(key, "%010d", val);

	return (key);
}

static const char*
timeKey(const char* text)
{
	int h, m;
	sscanf(text, "%d:%d", &h, &m);
	int t = h * 60 + m;
	static char key[16];
	sprintf(key, "%07d", t);

	return (key);
}


QString FileLVI::key(int column, bool dir) const
{
	return QString::null;
}


/////////////////////////////////////////////////////////////////////
// FileListView implementation
/////////////////////////////////////////////////////////////////////

FileListView::FileListView(QWidget *parent, const char* name)
	: QListView(parent, name)
{
	sortColumn = 1;
	increasing = FALSE;
}

FileListView::~FileListView()
{
}

void FileListView::buildList( QStrList * fileList )
{
	clear();
	
	QStrListIterator *sli = new QStrListIterator(*fileList);
	while(!sli->atLast())
	{
		FileLVI *flvi=new FileLVI(this);
		flvi->setText(0, sli->current());
		insertItem(flvi);
	}
}


