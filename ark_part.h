/*
  Copyright (C)
 
  2001: Macadamian Technologies Inc (author: Jian Huang, jian@macadamian.com)
 
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

#ifndef __ark_part_h__
#define __ark_part_h__

#include <kparts/part.h>
#include <kparts/browserextension.h>
#include <kparts/factory.h>
#include <arkwidget.h>
#include <kaction.h>

class ArkBrowserExtension: public KParts::BrowserExtension
{
    Q_OBJECT
public:
    ArkBrowserExtension( KParts::ReadOnlyPart * parent, const char * name = 0L );
public slots:
    void slotOpenURLRequested( const KURL & url );
};

class ArkPart: public KParts::ReadWritePart
{
    Q_OBJECT
public:
    ArkPart( QWidget *parentWidget, const char *widgetName, QObject *parent,
             const char *name, const QStringList &, bool readWrite );
    virtual ~ArkPart();

    static KAboutData* createAboutData();

public slots:
    void fixEnables();//rename to slotFixEnables()...
    void disableActions();
    void slotFilePopup( const QPoint & pPoint );
    void file_save_as();
    bool saveFile();
    bool closeURL();
signals:
    void fixActionState( const bool & bHaveFiles );
    void removeRecentURL( const QString & url );
    void addRecentURL(  const QString & url );
protected:
    virtual bool openFile();  //Opening an archive file
    void setupActions();
    void initialEnables();
    void init();

protected slots:
    void slotSaveProperties();

private:
    ArkWidget  *awidget;
    ArkBrowserExtension *m_ext;

    KAction *saveAsAction;
    KAction *addFileAction;
    KAction *addDirAction;
    KAction *extractAction;
    KAction *deleteAction;
    KAction *selectAllAction;
    KAction *viewAction;
    KAction *helpAction;
    KAction *openWithAction;
    KAction *selectAction;
    KAction *deselectAllAction;
    KAction *invertSelectionAction;
    KAction *popupEditAction;
    KAction *editAction;

    // the following have different enable rules from the above KActions
    KAction *popupViewAction;
    KAction *popupOpenWithAction;
    KAction *shellOutputAction;

};

#endif
