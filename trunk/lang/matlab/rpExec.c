/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Interface Helper Functions
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
   int             error;

   error = pipe(stdoutPipe);
   error = pipe(stderrPipe);

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

#include "mex.h"

void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[] )
{
   const mxArray      *cellArray;
   const mwSize       *dims;
   mwIndex             i1;
   char               *tmp;
   char              **commandArgs;
   int                 exitStatus = 0;
   int                *return1;
   int                 streamOutput;
   struct stdResults   commandOutput;

   commandOutput.stdoutBuffer = NULL;
   commandOutput.stderrBuffer = NULL;

   /* check proper input and output */
   if(nrhs != 2) {
      mexErrMsgIdAndTxt("rpExec:invalidNumInputs","Two input arguments required.");
      exitStatus = 1;
   }
   if(nlhs > 3) {
      mexErrMsgIdAndTxt("rpExec:maxlhs","Too many output arguments.");
      exitStatus = 1;
   }

   if(exitStatus == 0) {
      if       (!mxIsCell(prhs[0])) {
         mexErrMsgIdAndTxt("rpExec:inputNotCell","Command must be a cell array.");
         exitStatus = 1;
      } else if(mxGetNumberOfDimensions(prhs[0]) != 2) {
         mexErrMsgIdAndTxt("rpExec:inputNot2DCell","Command must be a 2 dimensional cell array.");
         exitStatus = 1;
      } else if(mxGetM(prhs[0]) != 1) {
         mexErrMsgIdAndTxt("rpExec:inputNotVector","Command must be a row vector.");
         exitStatus = 1;
      }
   }

   if(exitStatus == 0) {
      if(mxGetNumberOfElements(prhs[1]) != 1) {
         mexErrMsgIdAndTxt("rpExec:inputNotScalar","streamOutput must be a scaler.");
         exitStatus = 1;
      }
   }

   if(exitStatus == 0) {
      dims = mxGetDimensions(prhs[0]);
      commandArgs = mxCalloc(dims[1]+1,sizeof(char *));
      for(i1=0; i1<dims[1]; i1++) {
         cellArray = mxGetCell(prhs[0],i1);
         commandArgs[i1] = mxArrayToString(cellArray);
      }
      commandArgs[dims[1]] = NULL;

      commandOutput.streamOutput = mxGetScalar(prhs[1]);
      exitStatus = systemCommand(commandArgs[0],commandArgs,&commandOutput);

      for(i1=0; i1<dims[1]; i1++) {
         mxFree(commandArgs[i1]);
      }
      mxFree(commandArgs);
   }

   if(nlhs > 1) {
      plhs[0] = mxCreateNumericMatrix(1,1,mxINT32_CLASS,mxREAL);
      return1 = (int *)mxGetData(plhs[0]);
      return1[0] = exitStatus;
   }

   if(nlhs > 1) {
      plhs[1] = mxCreateString(commandOutput.stdoutBuffer);
   }

   if(nlhs > 2) {
      plhs[2] = mxCreateString(commandOutput.stderrBuffer);
   }

   mxFree(commandOutput.stdoutBuffer);
   mxFree(commandOutput.stderrBuffer);
}


