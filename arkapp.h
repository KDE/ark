/*

 ark -- archiver for the KDE project

 Copyright (C)

 2002: Helio Chissini de Castro <helio@conectiva.com.br>
 1999-2000: Corel Corporation (author: Emily Ezust  emilye@corel.com)
 1999: Francois-Xavier Duranceau duranceau@kde.org

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
#include <qdict.h>

// KDE includes
#include <kuniqueapplication.h>

class QString;
class QStringList;

class EqualKey
{
	public:
		bool operator()(const QString & str1, const QString & str2) const
		{
			return (str1 == str2);
		}
};


// This class follows the singleton pattern.
class ArkApplication : public KUniqueApplication
{
	Q_OBJECT
	public:
		virtual int newInstance();
		virtual ~ArkApplication() {}

		// keep track of windows so we know when to quit
		int windowCount() { return m_windowCount; }
		int addWindow() { ++m_windowCount; return m_windowCount; }
		void removeWindow() { --m_windowCount;}

		// keep track of open archive names so we don't open one twice
		// note that ArkWidget is not a pointer to const because raise()
		// requires later a pointer to nonconst.
		void addOpenArk(const KURL & _arkname, MainWindow * _ptr);
		void removeOpenArk(const KURL & _arkname);

		bool isArkOpenAlready(const KURL & _arkname);

		void raiseArk(const KURL & _arkname);

		// use this function to access data from other modules.
		static ArkApplication *getInstance();

	protected:
		ArkApplication();

	private:
		QWidget *m_mainwidget;  // to be the parent of all ArkWidgets
		int m_windowCount;

		QStringList openArksList;

		// a hash to obtain the window associated with a filename.
		// given a QString key, you get an ArkWidget * pointer.
		QDict<MainWindow> m_windowsHash;

		static ArkApplication *mInstance;
};

#endif // ARKAPP_H
