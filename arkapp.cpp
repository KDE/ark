#include <dcopclient.h>
#include <kdebug.h>
#include <kcmdlineargs.h>

#include <qfile.h>

#include "arkapp.h"
#include "arkwidget.h"

// copied from kdebase/kcontrol/kcontrol/main.cpp

ArkApplication::ArkApplication()
  : KUniqueApplication(), /* mArkData(0),*/
    mArkSettings(0)
{
  kdebug(0, 1601, "+ArkApplication::ArkApplication");
  kdebug(0, 1601, "-ArkApplication::ArkApplication");
}

int ArkApplication::newInstance()
{
  kdebug(0, 1601, "+ArkApplication::newInstance");

  QString Zip;
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  if (args->count() > 0) 
  {
     const char *arg = args->arg(0);
     if (arg[0] == '/')
     {
        Zip = QFile::decodeName(arg);
     }
     else
     {
        Zip = KCmdLineArgs::cwd() + "/" + QFile::decodeName(arg);
     }
  }
  args->clear();


  if (!Zip.isEmpty())
  {
      ArkWidget *toplevel = (ArkWidget *) mainWidget();
      toplevel->file_open(Zip);
  }

  kdebug(0, 1601, "-ArkApplication::newInstance");
  return 0;
}

#include "arkapp.moc"
