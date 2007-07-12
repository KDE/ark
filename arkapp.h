/*

 ark -- archiver for the KDE project
 Copyright (C) 2002 Helio Chissini de Castro <helio@conectiva.com.br>
 Copyright (C) 1999-2000 Corel Corporation (author: Emily Ezust <emilye@corel.com>)
 Copyright (C) 1999 Francois-Xavier Duranceau <duranceau@kde.org>

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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#ifndef ARKAPP_H
#define ARKAPP_H

#include "mainwindow.h"

// QT includes
#include <QHash>

// KDE includes
#include <KUniqueApplication>

class QString;
class QStringList;

class ArkApplication : public KUniqueApplication
{
	Q_OBJECT
	protected:
		ArkApplication();
	public:
		virtual ~ArkApplication() {}

		virtual int newInstance();

		// keep track of open archive names so we don't open one twice
		// note that ArkWidget is not a pointer to const because raise()
		// requires later a pointer to nonconst.
		void addOpenArk(const KUrl & _arkname, MainWindow * _ptr);
		void removeOpenArk(const KUrl & _arkname);

		bool isArkOpenAlready(const KUrl & _arkname);

		void raiseArk(const KUrl & _arkname);

		// use this function to access data from other modules.
		static ArkApplication *getInstance();


	private:
		QWidget *m_mainwidget;  // to be the parent of all ArkWidgets

		QStringList openArksList;

		// a hash to obtain the window associated with a filename.
		// given a QString key, you get a MainWindow * pointer.
		QHash<QString, MainWindow*> m_windowsHash;

		static ArkApplication *mInstance;
};

#endif // ARKAPP_H
