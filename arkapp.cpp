/*

 ark -- archiver for the KDE project

 Copyright (C)

 2002-2003: Helio Chissini de Castro <helio@conectiva.com.br>
 2003: Georg Robbers <Georg.Robbers@urz.uni-hd.de>
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

#include <dcopclient.h>
#include <kdebug.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <unistd.h>
#include <qfile.h>
#include <errno.h>


#include "arkapp.h"

ArkApplication *ArkApplication::mInstance = NULL;

// a helper function to follow a symlink and obtain the real filename
// Used in the ArkApplication functions that use the archive filename
// to make sure an archive isn't opened twice in different windows
// Now, readlink only gives one level so this function recurses.

static QString resolveFilename(const QString & _arkname)
{
	char *buff;
	int nread;
	int iter = 1;
	
	while ( true )
	{
		buff = new char[BUFSIZ*iter];
		nread = readlink( QFile::encodeName(_arkname), buff, BUFSIZ);
		if (-1 == nread)
		{
			if ( EINVAL == errno )  // not a symbolic link. Stopping condition.
			{
				delete [] buff;
				return _arkname;
			}
			else if ( ENAMETOOLONG == errno )
			{
				kdDebug(1601) << "resolveFilename: have to reallocate - name too long!" << endl;
				iter++;
				delete [] buff;
				continue;
			}
			else
			{
				delete [] buff;
				// the other errors will be taken care of already in simply
				// // opening the archive (i.e., the user will be notified)
				return "";
			}
		}
      else
		{
			buff[nread] = '\0';  // readlink doesn't null terminate
			QString name = QFile::decodeName( buff );
			delete [] buff;
			
			// watch out for relative pathnames
			if (name.at(0) != '/')
			{
				// copy the path from _arkname
				int index = _arkname.findRev('/');
				name = _arkname.left(index + 1) + name;
			}
			kdDebug(1601) << "Now resolve " << name << endl;
			
			return resolveFilename( name );
		}
	}
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
  : KUniqueApplication(), m_windowCount(0)
{
	m_mainwidget = new QWidget;
	setMainWidget(m_mainwidget);
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
            KCmdLineArgs::usage( i18n( "Wrong number of arguments specified" ) );
            return 0;
        }
    }

    if ( args->isSet( "add-to" ) && ( !args->isSet( "add" ) ) )
    {
        if ( args->count() < 2 )
        {
            KCmdLineArgs::usage( i18n( "You need to specify at least one file to be added to the archive." ) );
            return 0;
        }
        else
        {
            KURL::List URLList;
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
        KURL archiveName = args->url( 1 );  // the filename

        // if more than one file -> use directory name
        if ( !oneFile )
            archiveName.setPath( archiveName.directory() );

        archiveName.setFileName( archiveName.fileName() + extension );
        KURL::List URLList;
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
            KCmdLineArgs::usage( i18n( "You need to specify at least one file to be added to the archive." ) );
            return 0;
        }
        else
        {
            KURL::List URLList;
            for ( int c = 0; c < args->count() ; c++ )
                URLList.append( args->url( c ) );

            MainWindow *arkWin = new MainWindow();

            arkWin->addToArchive( URLList, args->cwd() );
            return 0;
        }
    }


    int i = 0;
    KURL url;
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
ArkApplication::addOpenArk(const KURL & _arkname, MainWindow *_ptr)
{
    QString realName;
    if( _arkname.isLocalFile() )
    {
        realName = resolveFilename( _arkname.path() );  // follow symlink
        kdDebug(1601) << " Real name of " << _arkname.prettyURL() << " is " << realName << endl;
    }
    else
        realName = _arkname.prettyURL();
    openArksList.append(realName);
    m_windowsHash.replace(realName, _ptr);
    kdDebug(1601) << "Saved ptr " << _ptr << " added open ark: " << realName << endl;
}

void
ArkApplication::removeOpenArk(const KURL & _arkname)
{
    QString realName;
    if ( _arkname.isLocalFile() )
        realName = resolveFilename( _arkname.path() );  // follow symlink
    else
        realName = _arkname.prettyURL();
    kdDebug(1601) << "Removing name " << _arkname.prettyURL() << endl;
    openArksList.remove(realName);
    m_windowsHash.remove(realName);
}

void
ArkApplication::raiseArk(const KURL & _arkname)
{
    kdDebug( 1601 ) << "ArkApplication::raiseArk " << endl;
    MainWindow *window;
    QString realName;
    if( _arkname.isLocalFile() )
        realName = resolveFilename(_arkname.path());  // follow symlink
    else
        realName = _arkname.prettyURL();
    window = m_windowsHash[realName];
    kdDebug(1601) << "ArkApplication::raiseArk " << window << endl;
    // raise didn't seem to be enough. Not sure why!
    // This might be annoying though.
    //window->hide();
    //window->show();
    window->raise();
}

bool
ArkApplication::isArkOpenAlready(const KURL & _arkname)
{
    QString realName;
    if ( _arkname.isLocalFile() )
        realName = resolveFilename(_arkname.path());  // follow symlink
    else
        realName = _arkname.prettyURL();
	return ( openArksList.findIndex(realName) != -1 );
}

#include "arkapp.moc"

