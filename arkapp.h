//  -*-C++-*-           emacs magic for .h files
#ifndef __ARKAPP_H__
#define __ARKAPP_H__

#include <kuniqueapp.h>
class ArkWidget;
class ArkSettings;
class ArkData;

class ArkApplication : public KUniqueApplication
{
  Q_OBJECT
public:
  ArkApplication(int argc, char *argv[], const QCString & rAppName);
  virtual int newInstance(QValueList<QCString> params);
  ArkWidget *mainWidget() { return mToplevel; }
  virtual ~ArkApplication() {}
private:
  ArkWidget *mToplevel;
  ArkData *mArkData; // for keeping uniqueapplication-wide hidden information
  ArkSettings *mArkSettings; // for storing user-defined options
};

#endif // __ARKAPP_H__
