//
//  ARKPROCESS -- A subclass of KProcess for providing 'popen'-like
//  capabilities

#ifndef __arkprocess_h__
#define __arkprocess_h__
#include <stdio.h>
#include <kprocess.h>

class ArkProcess : public KProcess {

public:

  /**
     With this method you can start a subprocess and be in contact with it
     through a pipe. comm's value may be Stdin, Stdout or Stderr. Don't forget
     that there can be only one. ;-) desc will contain the descriptor of the pipe.
     OK, it's a socket indeed.
  */
  bool startPipe(Communication comm = NoCommunication, FILE **stream = NULL);

  /**
     This is for test ride only
  */
  QStrList &getArguments();

  /**
     returns TRUE if the process is (still) considered to be running
     Note that it's a different implemention. It uses non-blocking wait()
     system call. Maybe I should have implemented normalExited() and so on.
  */
  bool isRunning();

private:

  /**
     This a modificated version of commSetupDoneP() only for internal use.
  */
  int commSetupDonePPipe();

};

#endif
