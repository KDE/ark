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

ArkApplication *ArkApplication::mInstance = NULL;

// a helper function to follow a symlink and obtain the real filename
// Used in the ArkApplication functions that use the archive filename
// to make sure an archive isn't opened twice in different windows
static QString resolveFilename( const QString & filename )
{
	QFileInfo fi( filename );

	while ( fi.isSymLink() )
	{
		fi = QFileInfo( fi.symLinkTarget() );
	}

	return fi.absoluteFilePath();
}

ArkApplication * ArkApplication::getInstance()
{
	if (mInstance == NULL)
	{
		mInstance = new ArkApplication();
	}
	return mInstance;
}

ArkApplication::ArkApplication()
  : KUniqueApplication()
{
	m_mainwidget = new QWidget;
}

int
ArkApplication::newInstance()
{

    // If we are restored by session management, we don't need to open
    // another window on startup.
    if (restoringSession()) return 0;

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    if ( args->isSet( "extract-to" ) )
    {
        if ( args->count() == 2 )
        {
            MainWindow *arkWin = new MainWindow();

            arkWin->extractTo( args->url( 0 ), args->url( 1 ), args->isSet( "guess-name" ) );
            return 0;
        }
        else
        {
            KCmdLineArgs::usageError( i18n( "Wrong number of arguments specified" ) );
            return 0;
        }
    }

    if ( args->isSet( "add-to" ) && ( !args->isSet( "add" ) ) )
    {
        if ( args->count() < 2 )
        {
            KCmdLineArgs::usageError( i18n( "You need to specify at least one file to be added to the archive." ) );
            return 0;
        }
        else
        {
            KUrl::List URLList;
            for ( int c = 0; c < args->count()-1 ; c++ )
                URLList.append( args->url( c ) );

            MainWindow *arkWin = new MainWindow();

            arkWin->addToArchive( URLList, args->cwd(), args->url( args->count()-1 ) );
            return 0;
        }
    }

    if ( args->isSet( "add" ) && args->isSet( "add-to" ) )   // HACK
    {
        bool oneFile = (args->count() == 2 ) ;

        QString extension = args->arg( 0 );
        KUrl archiveName = args->url( 1 );  // the filename

        // if more than one file -> use directory name
        if ( !oneFile )
            archiveName.setPath( archiveName.directory() );

        archiveName.setFileName( archiveName.fileName() + extension );
        KUrl::List URLList;
        for ( int c = 1; c < args->count(); c++ )
            URLList.append( args->url( c ) );

        MainWindow *arkWin = new MainWindow();

        arkWin->addToArchive( URLList, args->cwd(), archiveName, !oneFile );
        return 0;
    }


    if ( args->isSet( "add" ) && ( !args->isSet( "add-to" ) ) )
    {
        if ( args->count() < 1 )
        {
            KCmdLineArgs::usageError( i18n( "You need to specify at least one file to be added to the archive." ) );
            return 0;
        }
        else
        {
            KUrl::List URLList;
            for ( int c = 0; c < args->count() ; c++ )
                URLList.append( args->url( c ) );

            MainWindow *arkWin = new MainWindow();

            arkWin->addToArchive( URLList, args->cwd() );
            return 0;
        }
    }


    int i = 0;
    KUrl url;
    bool doAutoExtract = args->isSet("extract");
    bool tempFile = KCmdLineArgs::isTempFileSet();
    do
    {
        if (args->count() > 0)
        {
            url = args->url(i);
        }
        MainWindow *arkWin = new MainWindow();
        arkWin->show();
        if(doAutoExtract)
        {
            arkWin->setExtractOnly(true);
        }
        if (!url.isEmpty())
        {
            arkWin->openURL(url, tempFile);
        }

        ++i;
    } while  (i < args->count());

    args->clear();
    return 0;
}


void
ArkApplication::addOpenArk(const KUrl & _arkname, MainWindow *_ptr)
{
    QString realName;
    if( _arkname.isLocalFile() )
    {
        realName = resolveFilename( _arkname.path() );  // follow symlink
        kDebug(1601) << " Real name of " << _arkname.prettyUrl() << " is " << realName << endl;
    }
    else
        realName = _arkname.prettyUrl();
    openArksList.append(realName);
    m_windowsHash.remove(realName);
    m_windowsHash[realName] = _ptr;
    kDebug(1601) << "Saved ptr " << _ptr << " added open ark: " << realName << endl;
}

void
ArkApplication::removeOpenArk(const KUrl & _arkname)
{
    QString realName;
    if ( _arkname.isLocalFile() )
        realName = resolveFilename( _arkname.path() );  // follow symlink
    else
        realName = _arkname.prettyUrl();
    kDebug(1601) << "Removing name " << _arkname.prettyUrl() << endl;
    openArksList.removeAll(realName);
    m_windowsHash.remove(realName);
}

void
ArkApplication::raiseArk(const KUrl & _arkname)
{
    kDebug( 1601 ) << "ArkApplication::raiseArk " << endl;
    MainWindow *window;
    QString realName;
    if( _arkname.isLocalFile() )
        realName = resolveFilename(_arkname.path());  // follow symlink
    else
        realName = _arkname.prettyUrl();
    window = m_windowsHash[realName];
    kDebug(1601) << "ArkApplication::raiseArk " << window << endl;
    window->raise();
}

bool
ArkApplication::isArkOpenAlready(const KUrl & _arkname)
{
    QString realName;
    if ( _arkname.isLocalFile() )
        realName = resolveFilename(_arkname.path());  // follow symlink
    else
        realName = _arkname.prettyUrl();
	return ( openArksList.indexOf(realName) != -1 );
}

#include "arkapp.moc"

