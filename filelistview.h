#ifndef FILELISTVIEW_H
#define FILELISTVIEW_H

#include <qwidget.h>
#include <qlistview.h>

class FileLVI : public QListViewItem
{
public:
	FileLVI(QListView* lv) : QListViewItem(lv) {}
	FileLVI(QListViewItem* lv) : QListViewItem(lv) {}

//	virtual QString key(int column, bool) const;
};


class FileListView : public QListView
{
    Q_OBJECT

public:
	FileListView(QWidget* parent = 0, const char* name = 0);
	~FileListView();

public slots:


private:
	int sortColumn;
	bool increasing;
	
	virtual void setSorting(int column, bool inc = TRUE);
};

#endif
