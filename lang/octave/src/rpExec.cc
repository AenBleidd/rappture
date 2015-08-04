/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Octave Rappture Library Interface Helper Functions
 *
 * ======================================================================
 *  AUTHOR:  Steven Clark, Purdue University
 *  Copyright (c) 2015-2015  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>

int pid;
int parentSignal = 0;

void sighup(int sigNum) {
   if(pid != 0) {
      parentSignal = SIGHUP;
      kill(pid,SIGHUP);
   }
}

void sigint(int sigNum) {
   if(pid != 0) {
      parentSignal = SIGINT;
      kill(pid,SIGINT);
   }
}

void sigterm(int sigNum) {
   if(pid != 0) {
      parentSignal = SIGTERM;
      kill(pid,SIGTERM);
   }
}

#define MAXBUFFER 1024

struct stdResults {
   int streamOutput;
   char *stdoutBuffer;
   char *stderrBuffer;
};

int systemCommand(char *commandExe,
                  char **commandArgs,
                  struct stdResults *commandOutput)
{
   int             exitPid;
   int             status;
   int             exitStatus = 0;
   int             termSignal = 0;
   int             signalStatus = 1<<7;
   sighandler_t    matlabSighup;
   sighandler_t    matlabSigint;
   sighandler_t    matlabSigterm;
   int             stdoutPipe[2];
   int             stderrPipe[2];
   fd_set          fdSet;
   struct timeval  timeout;
   int             selectReturn;
   char            buffer[MAXBUFFER+1];
   int             stdoutEOF;
   int             stderrEOF;
   int             nRead;
   int             stdoutBufferSize = 32*MAXBUFFER;
   int             stderrBufferSize = 32*MAXBUFFER;
   char           *stdoutBuffer;
   char           *stderrBuffer;

   pipe(stdoutPipe);
   pipe(stderrPipe);

   if((pid = fork()) < 0) {
      perror("fork");
      return(1);
   }

   matlabSighup = signal(SIGHUP,sighup);
   matlabSigint = signal(SIGINT,sigint);
   matlabSigterm = signal(SIGTERM,sigterm);

   if(pid == 0) { /* child */
      dup2(stdoutPipe[1],STDOUT_FILENO);
      dup2(stderrPipe[1],STDERR_FILENO);
      close(stdoutPipe[0]);
      close(stderrPipe[0]);

      signal(SIGHUP,SIG_DFL);
      signal(SIGINT,SIG_DFL);
      signal(SIGTERM,SIG_DFL);

      execvp(commandExe,commandArgs);
   } else {       /* parent */
      close(stdoutPipe[1]);
      close(stderrPipe[1]);

      stdoutBuffer = (char *)calloc(stdoutBufferSize,sizeof(char));
      stderrBuffer = (char *)calloc(stderrBufferSize,sizeof(char));

      stdoutEOF = 0;
      stderrEOF = 0;
      while(!stdoutEOF || !stderrEOF) {
         FD_ZERO(&fdSet);
         if(!stdoutEOF)
            FD_SET(stdoutPipe[0],&fdSet);
         if(!stderrEOF)
            FD_SET(stderrPipe[0],&fdSet);
         timeout.tv_sec = 0;
         timeout.tv_usec = 500000;

         selectReturn = select(FD_SETSIZE,&fdSet,NULL,NULL,&timeout);
         if(selectReturn > 0) {
            if(FD_ISSET(stdoutPipe[0],&fdSet)) {
               nRead = read(stdoutPipe[0],buffer,MAXBUFFER);
               if(nRead > 0) {
                  buffer[nRead] = '\0';
                  if(strlen(stdoutBuffer)+nRead > stdoutBufferSize) {
                     stdoutBufferSize = 2*stdoutBufferSize;
                     stdoutBuffer = (char *)realloc(stdoutBuffer,stdoutBufferSize);
                  }
                  strcat(stdoutBuffer,buffer);
                  if(commandOutput->streamOutput)
                     fprintf(stdout,"%s",buffer);
               } else {
                  stdoutEOF = 1;
               }
            }
            if(FD_ISSET(stderrPipe[0],&fdSet)) {
               nRead = read(stderrPipe[0],buffer,MAXBUFFER);
               if(nRead > 0) {
                  buffer[nRead] = '\0';
                  if(strlen(stderrBuffer)+nRead > stderrBufferSize) {
                     stderrBufferSize = 2*stderrBufferSize;
                     stderrBuffer = (char *)realloc(stderrBuffer,stderrBufferSize);
                  }
                  strcat(stderrBuffer,buffer);
                  if(commandOutput->streamOutput)
                     fprintf(stderr,"%s",buffer);
               } else {
                  stderrEOF = 1;
               }
            }
         }
      }

      commandOutput->stdoutBuffer = stdoutBuffer;
      commandOutput->stderrBuffer = stderrBuffer;

      exitPid = waitpid(pid,&status,0);
      if       (WIFSIGNALED(status)) {
         termSignal = WTERMSIG(status);
         exitStatus = signalStatus + termSignal;
      } else if(WIFSTOPPED(status)) {
         termSignal = WSTOPSIG(status);
         exitStatus = signalStatus + termSignal;
      } else if(WIFEXITED(status)) {
         if(parentSignal > 0) {
            termSignal = parentSignal;
            exitStatus = signalStatus + termSignal;
         } else {
            exitStatus = WEXITSTATUS(status);
         }
      }
   }
   signal(SIGHUP,matlabSighup);
   signal(SIGINT,matlabSigint);
   signal(SIGTERM,matlabSigterm);

   return(exitStatus);
}

#include "octave/oct.h"
#include "octave/Cell.h"

#include "RpOctaveInterface.h"

DEFUN_DLD(rpExec,args,nlhs,
"-*- texinfo -*-\n\
@deftypefn {Function} {[@var{exitStatus}]} = rpExec(@var{command},@var{streamOutput})\n\
@deftypefnx {Function} {[@var{exitStatus}, @var{stdOutput}]} = rpExec(@var{command},@var{streamOutput})\n\
@deftypefnx {Function} {[@var{exitStatus}, @var{stdOutput}, @var{stdError}]} = rpExec(@var{command},@var{streamOutput})\n\
\n\
Execute @var{command} with the ability to terminate the\n\
process upon reception of a interrupt, hangup, or terminate\n\
signal. Doing so allows the process to terminated when the\n\
Rappture \"Abort\" button is pressed.  @var{command} should\n\
contain a set of strings that comprise the command to be\n\
executed.  If @var{streamOutput} equals 1 the stdout and\n\
stderr from @var{command} are piped back to the current process\n\
stdout and stderr descriptors as @var{command} executes.\n\n\
On output @var{exitStatus} indicates whether or not an error\n\
occurred.  @var{exitStatus} equals 0 indicates that no error\n\
occurred. If @var{stdOutput} is supplied it will contain a\n\
copy of stdout from @var{command}.  In the same manner if\n\
@var{stdError} is supplied it will contain a copy of stderr\n\
from @var{command}.\n\n\
Example:\n\n\
[exitStatus,stdOutput,stdError] = rpExec(@{\"submit\",\"--wallTime\",\"30\",\"lammps-12Feb14-serial\",\"-in\",\"lmp.in\"@},1);\n\
@end deftypefn")
{
   static std::string   who = "rpExec";
   int                  exitStatus = 0;
   octave_value_list    retval;
   int                  nrhs = args.length();
   Cell                 commandCell;
   char               **commandArgs;
   const char          *commandArg;
   struct stdResults    commandOutput;
   octave_idx_type      i1;

   commandOutput.stdoutBuffer = NULL;
   commandOutput.stderrBuffer = NULL;

   /* check proper input and output */
   if(nrhs != 2) {
      _PRINT_USAGE (who.c_str());
      exitStatus = 1;
   }
   if(nlhs > 3) {
      _PRINT_USAGE (who.c_str());
      exitStatus = 1;
   }

   if(exitStatus == 0) {
      if(!args(0).is_cell()) {
         _PRINT_USAGE (who.c_str());
         exitStatus = 1;
      }
   }

   if(exitStatus == 0) {
      if(!args(1).is_real_scalar()) {
         _PRINT_USAGE (who.c_str());
         exitStatus = 1;
      }
   }

   if(exitStatus == 0) {
      commandCell = args(0).cell_value();
      if(!error_state) {
         commandArgs = (char **)calloc(commandCell.numel()+1,sizeof(char *));
         for(i1=0; i1<commandCell.numel(); i1++) {
            commandArg = c(i1).string_value().c_str();
            commandArgs[i1] = (char *)calloc(strlen(commandArg)+1,sizeof(char));
            strcpy(commandArgs[i1],commandArg);
         }
         commandArgs[commandCell.numel()] = NULL;

         commandOutput.streamOutput = args(1).int_value();
         exitStatus = systemCommand(commandArgs[0],commandArgs,&commandOutput);

         for(i1=0; i1<commandCell.numel(); i1++) {
            free(commandArgs[i1]);
         }
         free(commandArgs);
      }
   }

   if(nlhs > 0) {
      retval(0) = octave_value(exitStatus);
   }

   if(nlhs > 1) {
      if(commandOutput.stdoutBuffer == NULL) {
         retval(1) = octave_value("",'"');
      } else {
         retval(1) = octave_value(commandOutput.stdoutBuffer,'"');
      }
   }

   if(nlhs > 2) {
      if(commandOutput.stderrBuffer == NULL) {
         retval(2) = octave_value("",'"');
      } else {
         retval(2) = octave_value(commandOutput.stderrBuffer,'"');
      }
   }

   free(commandOutput.stdoutBuffer);
   free(commandOutput.stderrBuffer);

   return retval;
}


