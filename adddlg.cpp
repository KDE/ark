/*

 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

 1999: Francois-Xavier Duranceau duranceau@kde.org
 1999-2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)
 2001: Corel Corporation (author: Michael Jarrett, michaelj@corel.com)

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

// QT includes
#include <qobjectlist.h>
#include <qlayout.h>

// KDE includes
#include <kdebug.h>
#include <klocale.h>

// Ark includes
#include "generalOptDlg.h"
#include "adddlg.h"
#include <qpushbutton.h>

/* QFileDialog is used instead of KFileDialog since (despite what the docs say),
   the layout management of KFileDialog bites if subclassing. Also, one can
   hope the directory selection behaves better here. */
AddDlg::AddDlg(AddTypes type, const QString & _sourceDir, ArkSettings *settings,
	       QWidget *parent, const char *name)
	: KFileDialog(_sourceDir, QString::null, parent, name, true),
    	  m_settings(settings)
{
	switch(type)
	{
	case Directory:
		setMode(KFile::Mode(KFile::Directory | KFile::ExistingOnly));
		setCaption(i18n("Select Directory to Add"));
	break;

	case File:
	default:
		setMode(KFile::Mode(KFile::Files | KFile::ExistingOnly));
		setCaption(i18n("Select Files to Add"));
	break;
	}
	kdDebug(1601) << "Made it one!" << endl;

	QObjectList *oList = queryList("QVBoxLayout");
	QVBoxLayout *layout = (QVBoxLayout *)oList->getFirst();
  delete oList;

	oList = queryList(0, "KFileDialog::mainWidget");
	QWidget *mainWidget = (QWidget *)oList->getFirst();
  delete oList;

  if ( 0 ) //(0 != layout && 0 != mainWidget)
      {
          kdDebug(1601) << "Made it there!" << endl;
          QPushButton *prefButton = new QPushButton(i18n("&Preferences..."), mainWidget);
          layout->addWidget(prefButton);
          connect(prefButton, SIGNAL(clicked()), this, SLOT(openPrefs()));
      }
  else
      Q_ASSERT(0);

}

/**
* @return String representing the directory, or blank if none.
*/
QString AddDlg::getDirectory()
{
	return selectedURL().path();
}

/**
* Opens the preferences dialog.
*/
void AddDlg::openPrefs()
{
	GeneralOptDlg dd(m_settings, this);
	dd.exec();
}

/**
* Hook into the GUI creation to add Preferences button.
* Hook to initGUI() in KFileDialog's constructor - a shabby KFD requires
* serious hacks to get our Preferences button added properly.
*/
void AddDlg::initGUI()
{
	kdDebug(1601) << "Made it here!" << endl;
	KFileDialog::initGUI();	// Makes sure our layouts are made

}

#include "adddlg.moc"
