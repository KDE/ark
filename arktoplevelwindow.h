//  -*- mode: c++; c-basic-offset: 4; -*-
/*
 
  ark -- archiver for the KDE project
 
  Copyright (C) 2002: Georg Robbers <Georg.Robbers@urz.uni-hd.de>
 
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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 
*/

#ifndef ARKTOPLEVELWINDOW_H
#define ARKTOPLEVELWINDOW_H

#define ARK_VERSION "1.9"

#include <qstring.h>
#include <qpopupmenu.h>

#include <kmainwindow.h>
#include <kparts/mainwindow.h>

class QWidget;
class QPoint;
class QStringList;
class QLabel;
class QListViewItem;
class QDragMoveEvent;
class QDropEvent;
class KMainWindow;
class KPopupMenu;
class KProcess;
class KConfig;
class KURL;
class KAction;
class KRecentFilesAction;
class KRun;
class KTempFile;

class Arch;
class ArkSettings;
class FileLVI;
class ArkWidget;
class ArkPart;

class ArkTopLevelWindow: public KParts::MainWindow
{
    Q_OBJECT
public:
    ArkTopLevelWindow( QWidget *parent=0, const char *name=0 );
    virtual ~ArkTopLevelWindow();

    void setExtractOnly ( bool b );

public slots:
    void file_newWindow();
    void file_new();
    void openURL( const KURL & url );
    void file_open();
    void file_reload();
    void file_save_as();
    void slotSetStatusBarSelectedFiles( const QString & text );
    void slotSetStatusBarText(  const QString & text );

    void editToolbars();
    void window_close();
    void file_quit();
    void file_close();
    void slotNewToolbarConfig();
    void slotConfigureKeyBindings();
    virtual void saveProperties( KConfig* config );
    virtual void readProperties( KConfig* config );
    void slotSaveProperties();
    void slotSaveOptions();
    void slotPreferences();
    void slotArchivePopup( const QPoint &pPoint);
    void slotRemoveRecentURL( const QString &url );
    void slotAddRecentURL( const QString &url );
    void slotFixActionState( const bool & bHaveFiles );
    void slotDisableActions();
    void slotAddOpenArk( const KURL & _arkname );
    void slotRemoveOpenArk( const KURL & _arkname );

protected:
    virtual bool queryClose(); // SM

private: // methods
    // disabling/enabling of buttons and menu items
    void setupActions();
    void setupStatusBar();
    void setupMenuBar();

    void newCaption(const QString & filename);
    bool arkAlreadyOpen( const KURL & url );

private: // data
    ArkPart *m_part;
    ArkWidget *m_widget; //the parts widget
    QLabel *m_pStatusLabelSelect; // How many files are selected - label
    QLabel *m_pStatusLabelTotal;  // How many files in archive - label
    KAction *newWindowAction;
    KAction *newArchAction;
    KAction *openAction;
    KAction *closeAction;
    KAction *reloadAction;
    KAction *saveAsAction;
    KRecentFilesAction *recent;
};

#endif /* ARKTOPLEVELWINDOW_H*/
