/*
	Copyright (C) 2001 Michael Jarrett <michaelj@corel.com>

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Library General Public
	License version 2 as published by the Free Software Foundation.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Library General Public License for more details.

	You should have received a copy of the GNU Library General Public License
	along with this library; see the file COPYING.LIB.  If not, write to
	the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*/

#ifndef KDIRSELECT_H
#define KDIRSELECT_H

class QWidget;
class QListViewItem;
class QPixmap;
class KListView;
class KURL;

#include <klistview.h>
#include <kdirlister.h>
#include <kfileitem.h>

/**
* An individual item for KDirSelect. Adds attribute to track whether
* it has listed itself or not.
* @author Michael Jarrett <michaelj@corel.com>
*/
class KDirSelectItem: public QListViewItem
{
public:
	KDirSelectItem(QListView *parent, QPixmap &icon, QString name, bool lock = false);
	KDirSelectItem(QListViewItem *parent, QPixmap &icon, QString name, bool lock = false);

	bool getListed() { return m_bListed; }
	void setListed(bool listed) { m_bListed = listed; }
	
	bool getDontList() { return m_bDontList; }
	void setDontList(bool lock) { m_bDontList = lock; }

private:
	bool m_bListed, m_bDontList;	
};


/**
* A widget for selecting an existing directory.
* This class provides the user with the ability to select a directory
* from a tree view. Until I write the code, I won't know more than that!
* @author Michael Jarrett <michaelj@corel.com>
*/
class KDirSelect : public KListView
{
	Q_OBJECT
public:
	KDirSelect(KURL &rootURL, QWidget *parent = 0, const char *name = 0);
	~KDirSelect();

	KURL getSelectedURL();

protected slots:
	void updateRoot();
	void updateBranch(QListViewItem *branch);

	void addItems(const KFileItemList &items);
	void addFinished();
	void subAddFinished();

protected:
	void doBranches();
	KURL makeURL(QListViewItem *branch);

private:
	KURL m_rootURL;
	QListViewItem *m_curBranch;
	KDirLister m_dirLister, m_subDirLister;

  class KDirSelectPrivate;
	KDirSelectPrivate *d;
};

#endif
