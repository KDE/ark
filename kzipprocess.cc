//
//  KZIPPROCESS -- A subclass of KProcess for providing 'popen'-like
//  capabilities

#include "kzipprocess.h"
#include <kprocess.h>
#include <stdio.h>
#include <stdlib.h>
// These includes are a little mysterious to me. No, they aren't!

#define _MAY_INCLUDE_KPROCESSCONTROLLER_
#include "kprocctrl.h"

#include "qapp.h"

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>

// #include "kzipprocess.moc"

/////////////////////////////
// public member functions //
/////////////////////////////

bool KZipProcess::startPipe(Communication comm, FILE** stream)
 {
   // Unique feature: there's no runmode, it's defaulted to DontCare. In theory the pipes
   // closed state can be determined bz getting an EOF.
 
   uint i;
   uint n = arguments.count();
   char **arglist;
 
// THESE FOUR LINES ARE COMMENTED OUT, BECAUSE THERE'S NO SOLUTION FOR
// CHECKING THE SUBPROCESS'S CEASE.
   if (isRunning()) {
     return FALSE;  // cannot start a process that is already running
     // or if no executable has been assigned
   }
 
   if ( (comm != Stdin) && (comm != Stdout) && (comm != Stderr) ) {
     return FALSE; // There can be only one (BIG GRIN)
   }
 
   status = 0;
 
   arglist = (char **)malloc( (n+1)*sizeof(char *));
   CHECK_PTR(arglist);
   for (i=0; i < n; i++)
     arglist[i] = arguments.at(i);
   arglist[n]= NULL;
 
   if (!setupCommunication(comm))
     debug("Could not setup Communication!");
 
   runs = TRUE; // Isn't needed two much
   pid = fork();
 
   if (0 == pid) {
     // The child process
 
     if(!commSetupDoneC())
	 debug("Could not finish comm setup in child!");
 
     // Matthias
     // if (run_mode == DontCare) // Of course!
     setpgid(0,0);
     
     for(int argiterator = 0; arglist[argiterator] != NULL; argiterator++)
     	debug(" Execvp's %dth arg is %s", argiterator, arglist[argiterator]);
     execvp(arglist[0], arglist);
     debug("Execvp hasn't succeed\n");
     exit(-1);
 
   } else if (-1 == pid) {
     // forking failed
 
     runs = FALSE;
     return FALSE;
 
   } else {
     // the parent continues here
 
     if (!commSetupDonePPipe())  // finish communication socket setup for the parent
	 debug("Could not finish comm setup in parent!");
 
     if (comm == Stdin)
	     *stream = fdopen(in[1], "w");
     else if (comm == Stdout)
	     *stream = fdopen(out[0], "r");
     else if (comm == Stderr)
	     *stream = fdopen(err[0], "r");
	     
 //  run mode is DontCare    
 //  if (run_mode == Block) {
 //	 waitpid(pid, &status, 0);
 //	 processHasExited(status);
 //  }
   }
   free(arglist);    
   return TRUE;
 }
 

QStrList &KZipProcess::getArguments()
 {
   return arguments;
 }
    	

bool KZipProcess::isRunning()
{
// Maybe I gotta use pid instead of getPid().
   int local_status;
   pid_t retval; 
   if(getPid() == 0)
   	return false;
   retval = wait4(getPid(), &local_status, WNOHANG, (struct rusage*)0);
   if(retval)
   {
   	status = local_status;
   	runs = false;
   }
   
   return(retval?false:true);
// 
}
//////////////////////////////
// private member functions //
//////////////////////////////

  
int KZipProcess::commSetupDonePPipe()
 {
   int ok = 1;
   struct linger so;
 
   if (communication != NoCommunication) {
     if (communication & Stdin)
	 close(in[0]);
     if (communication & Stdout)
	 close(out[1]);
     if (communication & Stderr)
	 close(err[1]);
 
     if (communication & Stdin) {
	 ok &= !setsockopt(in[1], SOL_SOCKET, SO_LINGER, (char*)&so,
	 sizeof(so));
     }
//	They'll make the stream non-blocked. We may put it in the Arch classes.
//	No one will complain. ( 8-> )

//	fcntl(*desc, F_SETFL, FNDELAY);


//    if (communication & Stdout)
//    {
//    	debug("I'm gonna make stdout non-blocked");
// 		ok &= (-1 != fcntl(out[0], F_SETFL, FNDELAY));
//    }
//    if (communication & Stderr)
// 		ok &= (-1 != fcntl(err[0], F_SETFL, FNDELAY));
    }
   return ok;
 }
