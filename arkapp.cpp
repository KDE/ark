#include <kapp.h>
#include <dcopclient.h>
#include <sys/param.h>
#include <kdebug.h>
#include "arkapp.h"
#include "arkwidget.h"

// copied from kdebase/kcontrol/kcontrol/main.cpp

ArkApplication::ArkApplication(int argc, char *argv[],
			       const QCString & rAppName)
  : KUniqueApplication(argc, argv, rAppName), mToplevel(0), mArkData(0),
    mArkSettings(0)
{
  kdebug(0, 1601, "+ArkApplication::ArkApplication");
  kdebug(0, 1601, "-ArkApplication::ArkApplication");
}

int ArkApplication::newInstance(QValueList<QCString> params)
{
  kdebug(0, 1601, "+ArkApplication::newInstance");

  QValueList<QCString>::Iterator it = params.begin();
  QString Zip("");

  it++; // skip program name
  for (; it != params.end(); it++)
  {
    kdebug(0, 1601, "Parsing params..");
      QString strParam = *it;
      if (strParam.left(1) == '/')
      {
	  Zip = *it;
	  break;
      }
      else if (strParam.left(1) == '-') // not that we have any options yet!
      {
      }
      else
      {
	  char currentWD[MAXPATHLEN];
	  getcwd(currentWD, MAXPATHLEN);
	  (Zip = currentWD).append("/").append(*it);
	  break;
      }
  }

  if (!Zip.isEmpty())
  {
      mToplevel->file_open(*it);
  }

  kdebug(0, 1601, "-ArkApplication::newInstance");
  return 0;
}

#include "arkapp.moc"
