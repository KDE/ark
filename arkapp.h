//  -*-C++-*-           emacs magic for .h files
#ifndef __ARKAPP_H__
#define __ARKAPP_H__

#include <kuniqueapp.h>
#include "arkwidget.h"

class ArkSettings;
class ArkSettings;

class ArkApplication : public KUniqueApplication
{
  Q_OBJECT
public:
  ArkApplication();
  virtual int newInstance();
  virtual ~ArkApplication() {}
  void addWindow(ArkWidget *_arkWin) { windowList->append(_arkWin); }
  void removeWindow(ArkWidget *_arkWin) { windowList->removeRef(_arkWin); }
  int windowCount() { return windowList->count(); }
private:
//  ArkData *mArkData; // for keeping uniqueapplication-wide hidden information
  ArkSettings *mArkSettings; // for storing user-defined options
  QList<ArkWidget> *windowList;
};

#endif // __ARKAPP_H__
