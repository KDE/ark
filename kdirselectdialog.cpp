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

#include <qlayout.h>
#include <qpushbutton.h>
#include <kdialog.h>
#include <klocale.h>

#include "kdirselect.h"
#include "kdirselectdialog.h"

/**
* The constructor. Nothing out of the ordinary here.
*/
KDirSelectDialog::KDirSelectDialog(KURL &rootURL,
	QWidget *parent, const char *name) : KDialog(parent, name, true)
{
	setCaption(i18n("Directories"));

	// Create buttons
	QPushButton *okButton = new QPushButton(i18n("&OK"), this);
	QPushButton *cancelButton = new QPushButton(i18n("&Cancel"), this);
	okButton->setDefault(true);

	connect(okButton, SIGNAL(pressed()), this, SLOT(accept()));
	connect(cancelButton, SIGNAL(pressed()), this, SLOT(reject()));

	// Create dir list
	m_dirList = new KDirSelect(rootURL, this);

	// Setup layouts
	m_mainLayout = new QVBoxLayout(this, marginHint(), spacingHint());
	m_mainLayout->addWidget(m_dirList, 1);
	
	m_buttonLayout = new QHBoxLayout(spacingHint());
	m_mainLayout->addLayout(m_buttonLayout);
	m_buttonLayout->addStretch(1);
	
	m_buttonLayout->addWidget(cancelButton);
	m_buttonLayout->addWidget(okButton);
}


/**
* Deletes the layouts used in the dialog.
*/
KDirSelectDialog::~KDirSelectDialog()
{
	// Deletes layout, which will delete child layouts
	// The buttons will be killed by QWidget.
	hide();
	delete m_buttonLayout; m_buttonLayout = 0;
	delete m_mainLayout; 	m_mainLayout = 0;
}


/**
* Returns the currently-selected URL, or a blank URL if none is selected.
* @return The currently-selected URL, if one was selected.
*/
KURL KDirSelectDialog::getURL()
{
	return m_dirList->getSelectedURL();
}


/**
* Creates a KDirSelectDialog, and returns the result.
* @param root The root from which to start enumerating directories.
* The tree will display this directory and subdirectories of it.
* @return The URL selected, or an empty URL if the user cancelled
* or no URL was selected.
*/
KURL KDirSelectDialog::selectDirectory(KURL root, QWidget *parent)
{
	KDirSelectDialog myDialog(root, parent, "kdirselectdialog");
	if(myDialog.exec())
		return myDialog.getURL();
	else
		return KURL();
}


#include "kdirselectdialog.moc"
