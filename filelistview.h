#ifndef FILELISTVIEW_H
#define FILELISTVIEW_H

#include <qlistview.h>
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
	
public slots:


private:
	int sortColumn;
	bool increasing;
	
	virtual void setSorting(int column, bool inc = TRUE);
};

#endif
