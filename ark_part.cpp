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

#include "ark_part.h"

#include <klocale.h>
#include <kinstance.h>
#include <kaction.h>
#include <kaboutdata.h>


#include "arkwidgetpart.h"

extern "C"
{
    /**
     * This function is the 'main' function of this part.  It takes
     * the form 'void *init_lib<library name>()'.  It always returns a
     * new factory object
     */
    void *init_libark()
    {
	KGlobal::locale()->insertCatalogue("ark");
        return new ArkFactory;
    }
};

/********************************************************************
* We need one static instance of the factory for our C 'main'
* function
*/
KInstance *ArkFactory::s_instance = 0L;

ArkFactory::ArkFactory()
{
    KGlobal::locale()->insertCatalogue( QString::fromLatin1("ark") );
}

ArkFactory::~ArkFactory()
{
    if (s_instance)
    {
        delete s_instance->aboutData();
        delete s_instance;
    }

    s_instance = 0;
}


/*********************************************************************
*                             create
*
*This function is called each time when ark KParts component is needed
*It's responsible for instantiating a new view object and returning it.
*/
KParts::Part *ArkFactory::createPartObject(QWidget *parentWidget, const char *widgetName,
                                           QObject *parent, const char *name, const char*,
                                           const QStringList& )
{
    KParts::Part *obj = new ArkPart(parentWidget, widgetName, parent, name);
    return obj;
}


/**************************************************************************
*                           instance
*
*This function is responsible for instantiating an object of type KInstance
*/
KInstance *ArkFactory::instance()
{
    if ( !s_instance )
        s_instance = new KInstance( aboutData() );
    return s_instance;
}

KAboutData *ArkFactory::aboutData()
{
  KAboutData *about = new KAboutData("ark", I18N_NOOP("ark"), 
                                     "1.0",
				     I18N_NOOP("Ark KParts Component"), 
                                     KAboutData::License_GPL,
				     "(c) 1997-2001, The Various Ark Developers");
  about->addAuthor("Robert Palmbos",0, "palm9744@kettering.edu");
  about->addAuthor("Francois-Xavier Duranceau",0, "duranceau@kde.org");
  about->addAuthor("Corel Corporation (author: Emily Ezust)",0,
		   "emilye@corel.com");
  about->addAuthor("Corel Corporation (author: Michael Jarrett)", 0,
		   "michaelj@corel.com");
  about->addAuthor("Jian Huang");
  about->addAuthor( "Roberto Teixeira", 0, "maragato@kde.org" );

  return about;
}

/**************************************************************************
*                           ArkPart
*
*This constructor is responsible for initializing an object of ark KParts
*component and creating its actions
*/
ArkPart::ArkPart(QWidget *parentWidget, const char *widgetName,
                 QObject *parent, const char *name)
    : KParts::ReadOnlyPart(parent, name)
{
    setInstance(ArkFactory::instance());

    //create an ark widget
    awidget = new  ArkWidgetPart (parentWidget, widgetName);
    awidget->setFocus();
    setWidget(awidget);

    //create and connect to different actions
    m_extractAction = new KAction(i18n("&Extract"), "ark_extract",
                               0, this,
                               SLOT(slotExtract()), actionCollection(),
                               "extract");

    m_viewAction = new KAction(i18n("&View"), "ark_view",
                               0, this,
                               SLOT(slotView()), actionCollection(),
                               "view");

    m_extension = new ArkBrowserExtension(this);
    setXMLFile("ark_part.rc");

    m_extractAction->setEnabled(false);
    m_viewAction->setEnabled(false);

    connect(awidget, SIGNAL(toKpartsView(int, int)), this, SLOT(slotEnableView(int, int)) );
}

ArkPart::~ArkPart(){}

bool ArkPart::openFile()
{
    awidget->file_open(m_file, m_url);
    m_viewAction->setEnabled(false);
    if(awidget->goodFileType==1)
      m_extractAction->setEnabled(true);
    return true;
}

void ArkPart::slotExtract()
{
    awidget->action_extract();
}

void ArkPart::slotView()
{
  awidget->action_view();
}

void ArkPart::slotEnableView(int fNum, int selectedfNum)
{
  m_viewAction->setEnabled(fNum>0&&selectedfNum==1);
}


ArkBrowserExtension::ArkBrowserExtension(ArkPart *parent)
    : KParts::BrowserExtension(parent, "ArkBrowserExtension")
{
}

ArkBrowserExtension::~ArkBrowserExtension()
{
}
#include "ark_part.moc"
