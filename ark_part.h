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
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/


/*The Ark KParts component needs to construct two classes:            *
 *A factory class derived from KLibFactory and a view class derived   * 
 *from KParts::ReadOnlyPart. The factory object is responsible for    * 
 *instantiating the component and returning a pointer to it.          *
 *The ArkPart constructor is responsible for initializing the internal*
 *variables as well as the "workhorse" object.                        */
 

#ifndef __ark_part_h__
#define __ark_part_h__

#include <kparts/part.h>
#include <kparts/browserextension.h>
#include <kparts/factory.h>

class QObject;
class QString;
class QStringList;
class KAction;
class KInstance;
class KLibFactory;
class KAboutData;

class ArkWidgetPart;

class ArkBrowserExtension;

class ArkFactory : public KParts::Factory
{
    Q_OBJECT
public:
    ArkFactory();
    virtual ~ArkFactory();
    virtual KParts::Part* createPartObject(QWidget *parentWidget, const char *widgetName,
                            QObject* parent = 0, const char* name = 0,
                            const char* classname = "QObject",
                            const QStringList &args = QStringList());
    static KInstance *instance();
    static KAboutData *aboutData();

private:
    static KInstance *s_instance;
};

class ArkPart: public KParts::ReadOnlyPart
{
    Q_OBJECT
public:
    ArkPart(QWidget *parentWidget, const char *widgetName,
            QObject *parent = 0, const char *name = 0);
    virtual ~ArkPart();

protected:
    virtual bool openFile();  //Opening an archive file

protected slots:
    void slotExtract();   //extracting an archive file 
    void slotView();      //viewing a file in an archive file
    void slotEnableView(int fNum, int selectedfNum);

private:
    ArkWidgetPart  *awidget;  
    ArkBrowserExtension *m_extension;

    KAction *m_extractAction;
    KAction *m_viewAction;
};

class ArkBrowserExtension : public KParts::BrowserExtension
{
    Q_OBJECT
    friend class ArkPart;
public:
    ArkBrowserExtension(ArkPart *parent);
    virtual ~ArkBrowserExtension();
};
#endif
