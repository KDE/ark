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


#include <klocale.h>

#include "kdirselect.h"

class KDirSelect::KDirSelectPrivate
{
public:
	QPixmap dirIcon;
};


KDirSelectItem::KDirSelectItem(QListView *parent, QPixmap &icon, QString name,
                               bool lock)
	: QListViewItem(parent, name), m_bListed(false), m_bDontList(lock)
{
	setPixmap(0, icon);
}


KDirSelectItem::KDirSelectItem(QListViewItem *parent, QPixmap &icon, QString name,
                               bool lock)
	: QListViewItem(parent, name), m_bListed(false), m_bDontList(lock)
{
	setPixmap(0, icon);
}


/**
* Create a new directory selection control.
* Constructor will automatically enumerate the first two levels of
* directories into the tree.
* @param rootURL Root of the tree to draw.
*/
KDirSelect::KDirSelect(KURL &rootURL, QWidget *parent, const char *name)
	: KListView(parent, name), m_rootURL(rootURL), m_curBranch(0)
{
	d = new KDirSelectPrivate;

	d->dirIcon = SmallIcon(QString::fromLatin1("folder"));

	setRootIsDecorated(true);	
	addColumn(i18n("Directory"));

	// Update the directory list
	m_dirLister.setDirOnlyMode(true);
	connect(&m_dirLister, SIGNAL(newItems(const KFileItemList &)), this,
		SLOT(addItems(const KFileItemList &)));
	connect(&m_dirLister, SIGNAL(completed()), this, SLOT(addFinished()));

	m_subDirLister.setDirOnlyMode(true);
	connect(&m_subDirLister, SIGNAL(newItems(const KFileItemList &)), this,
		SLOT(addItems(const KFileItemList &)));
	connect(&m_subDirLister, SIGNAL(completed()), this, SLOT(subAddFinished()));
	
	connect(this, SIGNAL(expanded(QListViewItem *)),
		SLOT(updateBranch(QListViewItem *)));

	updateRoot();
}

KDirSelect::~KDirSelect()
{
	delete d;
	d = 0;
}


/**
* Gets the currently-selected URL.
* @return The current URL, or an empty URL if none is selected.
*/
KURL KDirSelect::getSelectedURL()
{
	QListViewItem *current = currentItem();
	if(0 == current)
		return KURL();

	return makeURL(current);
}

/**
* Update the entire tree. Kills whatever was previously in there and
* will enumerate the first two subdirectories
*/
void KDirSelect::updateRoot()
{
	clear();
	m_curBranch = 0;
	m_dirLister.openURL(m_rootURL, true);
}

/**
* Used in branch updating. The branch's children are already there,
* but we need the children's children to expand it further.
*/
void KDirSelect::updateBranch(QListViewItem *branch)
{

	m_curBranch = branch->firstChild();
	doBranches();
}


/**
* Process incoming file items from our dirlister.
*/
void KDirSelect::addItems(const KFileItemList &items)
{
	for(KFileItemListIterator it(items); 0 != it.current(); ++it)
	{
		if(0 == m_curBranch)
			new KDirSelectItem(this, d->dirIcon, (*it)->name(),
                         !(*it)->isReadable());
		else
		{
			new KDirSelectItem(m_curBranch, d->dirIcon, (*it)->name(),
                         !(*it)->isReadable());
		}
	}		
}


/**
* Slot to indicate we are finished the first level, and will need to
* enumerate the next level.
*/
void KDirSelect::addFinished()
{
	m_curBranch = firstChild();
	doBranches();
}


/**
* We are done a branch, go to the next one, if any, and list it.
*/
void KDirSelect::subAddFinished()
{
	m_curBranch = m_curBranch->nextSibling();
	doBranches();
}


/**
* Builds a URL, and activates the subdirectory lister to get the
* next level of directories.
*/
void KDirSelect::doBranches()
{
	// TODO: Consider this. If just ignore the branch instead of clearing,
	// the tree becomes much faster. But then we must rely on our
	// DirLister for updates, which may only work locally.
	// AND this assumes I write the code for the latter!
	while(0 != m_curBranch && ( ((KDirSelectItem *)m_curBranch)->getListed()
        || ((KDirSelectItem *)m_curBranch)->getDontList() ))
		m_curBranch = m_curBranch->nextSibling();

	if(0 == m_curBranch)	
		return;	// We've enumerated all the directories.

	((KDirSelectItem *)m_curBranch)->setListed(true);
	m_subDirLister.openURL(makeURL(m_curBranch), true);
}


/**
* Creates a URL from a QListViewItem from using the root URL and appending
* down the tree to our current item.
* @param branch The directory for which one wants the URL.
* @return A URL consisting of the root URL plus the path to branch.
*/
KURL KDirSelect::makeURL(QListViewItem *branch)
{
	// Build up our path
	QStringList urlBranches;
	for(QListViewItem *b = branch; 0 != b;
		  b = b->parent())
	{
		urlBranches.prepend(b->text(0));
	}
		
	KURL loadURL(m_rootURL);
	for (QStringList::Iterator it = urlBranches.begin();
			 it != urlBranches.end(); ++it )
	{
		loadURL.addPath((*it));
	}
	return loadURL;
}

#include "kdirselect.moc"
