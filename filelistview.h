#ifndef FILELISTVIEW_H
#define FILELISTVIEW_H

#include <qlistview.h>
#include <qstringlist.h>
#include <qwidget.h>

class FileListView;

class FileLVI : public QListViewItem
{
public:
	FileLVI(QListView* lv) : QListViewItem(lv), parent(lv) {}
//	FileLVI(QListViewItem* lv) : QListViewItem(lv), parent(0) {}
	QString getFileName();

	virtual QString key(int column, bool) const;
private:
    QListView *parent;
	
};


class FileListView : public QListView
{
    Q_OBJECT

public:
	FileListView(QWidget* parent = 0, const char* name = 0);
	~FileListView();
	FileLVI *currentItem() {return ((FileLVI *) QListView::currentItem());}
	QStringList * selectedFilenames() const;
	uint count();
	bool isSelectionEmpty();
	
public slots:


private:
	int sortColumn;
	bool increasing;
	
	virtual void setSorting(int column, bool inc = TRUE);
};

#endif
