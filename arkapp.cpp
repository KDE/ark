/*

 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

 1999: Francois-Xavier Duranceau duranceau@kde.org
 1999-2000: Corel Corporation (author: Emily Ezust  emilye@corel.com)

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

#include <dcopclient.h>
#include <kdebug.h>
#include <kcmdlineargs.h>
#include <unistd.h>
#include <qfile.h>
#include <errno.h>
#include <kmainwindow.h>
#include <kio/jobclasses.h>


#include "arkapp.h"
#include "arkwidget.h"

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
  while (1)
    {
      buff = new char[BUFSIZ*iter];
      nread = readlink( QFile::encodeName(_arkname),
			buff, BUFSIZ);
      if (-1 == nread)
	{
	  if (EINVAL == errno)  // not a symbolic link. Stopping condition.
	    return _arkname;
	  else if (ENAMETOOLONG == errno)
	    {
	      kdDebug(1601) << "resolveFilename: have to reallocate - name too long!" << endl;
	      iter++;
	      delete buff;
	      continue;
	    }
	  else
	    {
	      // the other errors will be taken care of already in simply
	      // opening the archive (i.e., the user will be notified)
	      return "";
	    }
	}
      else
	{
	  buff[nread] = '\0';  // readlink doesn't null terminate
	  QString name = QFile::decodeName(buff);
	  delete buff;

	  // watch out for relative pathnames
	  if (name.at(0) != '/')
	    {
	      // copy the path from _arkname
	      int index = _arkname.findRev('/');
	      name = _arkname.left(index + 1) + name;
	    }
	  //kdDebug(1601) << "Now resolve " << (const char *)name << endl;
	  return resolveFilename(name);
	}
    }
}



ArkApplication * ArkApplication::getInstance()
{
  if (mInstance == NULL)
    mInstance = new ArkApplication();
  return mInstance;
}

ArkApplication::ArkApplication()
  : KUniqueApplication(), m_windowCount(0), m_isSessionRestored(isRestored())
{
  kdDebug(1601) << "+ArkApplication::ArkApplication" << endl;
  m_mainwidget = new QWidget;
  setMainWidget(m_mainwidget);
  kdDebug(1601) << "-ArkApplication::ArkApplication" << endl;
}

int ArkApplication::newInstance()
{
  kdDebug(1601) << "+ArkApplication::newInstance" << endl;

  // If we are restored by session management, we don't need to open
  // another window on startup.
  if ( m_isSessionRestored )
  {
    // But next invocations must still come through.
    // NOTE: IMHO this should be handled by KUniqueApplication itself
    m_isSessionRestored = false;
    kdDebug(1601) << "-ArkApplication::newInstance no new window since restored by SM" << endl;
    return 0;
  }

  QString Zip;

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  KURL url;

  int i = 0;
  bool doAutoExtract = args->isSet("extract");

  do {
    if (args->count() > 0)
      url = args->url(i);
    ArkWidget *arkWin = new ArkWidget(m_mainwidget);
    arkWin->show();
    arkWin->resize(640, 300);
    if(doAutoExtract)
        arkWin->setExtractOnly(true);

    if (!url.isEmpty())
      {
	arkWin->file_open(url);
      }
    kdDebug(1601) << "-ArkApplication::newInstance" << endl;
    ++i;
  } while  (i < args->count());
  args->clear();
  return 0;
}


void ArkApplication::addOpenArk(const QString & _arkname,
				ArkWidget *_ptr)
{
  kdDebug(1601) << "+ArkApplication::addOpenArk" << endl;
  QString realName = resolveFilename(_arkname);  // follow symlink
  kdDebug(1601) << "---------------- Real name of " << _arkname << " is " << realName << endl;
  openArksList.append(realName);
  m_windowsHash.replace(realName, _ptr);
  kdDebug(1601) << "---------------Saved ptr " << _ptr << endl;
  kdDebug(1601) << "-ArkApplication::addOpenArk" << endl;
}

void ArkApplication::removeOpenArk(const QString & _arkname)
{
  kdDebug(1601) << "+ArkApplication::removeOpenArk" << endl;
  QString realName = resolveFilename(_arkname);  // follow symlink
  kdDebug(1601) << "Removing name " << _arkname << endl;
  openArksList.remove(realName);
  m_windowsHash.remove(realName);
  kdDebug(1601) << "-ArkApplication::removeOpenArk" << endl;
}

void ArkApplication::raiseArk(const QString & _arkname)
{
  ArkWidget *window;
  QString realName = resolveFilename(_arkname);  // follow symlink
  window = m_windowsHash[realName];
  kdDebug(1601) << "ArkApplication::raiseArk " << window << endl;
  // raise didn't seem to be enough. Not sure why!
  // This might be annoying though.
  window->hide();
  window->show();
  window->raise();
}


bool ArkApplication::isArkOpenAlready(const QString & _arkname)
{
  QString realName = resolveFilename(_arkname);  // follow symlink
  return (openArksList.findIndex(realName) != -1);
}


#include "arkapp.moc"
