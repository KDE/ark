//  -*-C++-*-           emacs magic for .h files
#ifndef __ARKAPP_H__
#define __ARKAPP_H__

#include <kuniqueapp.h>

class ArkWidget;
class ArkSettings;
class ArkSettings;

class ArkApplication : public KUniqueApplication
{
  Q_OBJECT
public:
  ArkApplication();
  virtual int newInstance();
  virtual ~ArkApplication() {}
private:
//  ArkData *mArkData; // for keeping uniqueapplication-wide hidden information
  ArkSettings *mArkSettings; // for storing user-defined options
};

#endif // __ARKAPP_H__
