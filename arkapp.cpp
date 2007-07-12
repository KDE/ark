/*

 ark -- archiver for the KDE project

 Copyright (C) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 Copyright (C) 2002-2003 Helio Chissini de Castro <helio@conectiva.com.br>
 Copyright (C) 2003 Georg Robbers <Georg.Robbers@urz.uni-hd.de>
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
#include "arkapp.h"

#include <QFile>
#include <QFileInfo>

#include <KDebug>
#include <KCmdLineArgs>
#include <KLocale>

ArkApplication *ArkApplication::m_instance = NULL;

// a helper function to follow a symlink and obtain the real filename
// Used in the ArkApplication functions that use the archive filename
// to make sure an archive isn't opened twice in different windows
static QString resolveFilename( const KUrl & url )
{
	if ( !url.isLocalFile() )
	{
		return url.prettyUrl();
	}
	else
	{
		QFileInfo fi( url.path() );

		while ( fi.isSymLink() )
		{
			fi = QFileInfo( fi.symLinkTarget() );
		}

		return fi.absoluteFilePath();
	}
}

ArkApplication * ArkApplication::getInstance()
{
	if ( !m_instance )
	{
		m_instance = new ArkApplication;
	}
	return m_instance;
}

ArkApplication::ArkApplication()
  : KUniqueApplication()
{
}

int ArkApplication::newInstance()
{
	// If we are restored by session management, we don't need to open
	// another window on startup.
	if ( restoringSession() )
		return 0;

	KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

	int i = 0;
	bool tempFile = KCmdLineArgs::isTempFileSet();
	do
	{
		KUrl url;
		if (args->count() > 0)
		{
			url = args->url(i);
		}
		MainWindow *arkWin = new MainWindow();
		arkWin->show();
		if ( !url.isEmpty() )
		{
			arkWin->openURL(url, tempFile);
		}

		++i;
	} while  ( i < args->count() );

	args->clear();
	return 0;
}


void ArkApplication::addOpenArk(const KUrl & url, MainWindow *_ptr)
{
	m_windowsHash.insert( resolveFilename( url ), _ptr );
}

void ArkApplication::removeOpenArk( const KUrl & url )
{
	m_windowsHash.remove( resolveFilename( url ) );
}

void ArkApplication::raiseArk( const KUrl & url )
{
	QString realName = resolveFilename( url );
	if ( m_windowsHash.contains( realName ) )
		m_windowsHash[realName]->raise();
}

bool ArkApplication::isArkOpenAlready( const KUrl & url )
{
	return m_windowsHash.contains( resolveFilename( url ) );
}

#include "arkapp.moc"

