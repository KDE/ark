//  -*-C++-*-           emacs magic for .h files

#ifndef __ARKAPP_H__
#define __ARKAPP_H__

#include <kuniqueapp.h>
#include "arkwidget.h"

class ArkSettings;

// This class follows the singleton pattern.

class ArkApplication : public KUniqueApplication
{
  Q_OBJECT
public:
  virtual int newInstance();
  virtual ~ArkApplication() {}
  void addWindow(ArkWidget *_arkWin) { windowList->append(_arkWin); }
  void removeWindow(ArkWidget *_arkWin) { windowList->removeRef(_arkWin); }
  int windowCount() { return windowList->count(); }

  // use this function to access data from other modules.
  static ArkApplication *getInstance();
protected:
  ArkApplication();
private:
  QList<ArkWidget> *windowList;
  static ArkApplication *mInstance; 
};

#endif // __ARKAPP_H__
