#ifndef FILELISTVIEW_H
#define FILELISTVIEW_H

#include <qlistview.h>
#include <qstringlist.h>
#include <qwidget.h>

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
