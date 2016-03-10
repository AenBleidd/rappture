# ----------------------------------------------------------------------
#
# ======================================================================
#  AUTHOR:  Derrick S. Kearney, Purdue University
#  AUTHOR:  Steve Clark, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
# ======================================================================

import sys
import os
import re
import subprocess
import shlex
import select
import signal
import traceback
import time

commandPid = 0

def sig_handler(signalType, frame):
    global commandPid
    if commandPid:
        os.kill(commandPid,signal.SIGTERM)


def executeCommand(command,
                   stdin=None,
                   streamOutput=False):
    global commandPid

    exitStatus = 0
    fpStdin = None
    outData = []
    errData = []

    try:
       sig_INT_handler = signal.signal(signal.SIGINT,sig_handler)
       sig_HUP_handler = signal.signal(signal.SIGHUP,sig_handler)
       sig_TERM_handler = signal.signal(signal.SIGTERM,sig_handler)
    except ValueError:
#      happens when used in a thread
       pass
    except:
       print traceback.format_exc()

    if stdin == None:
       commandStdin = None
    else:
       if   isinstance(stdin,file):
          commandStdin = stdin
       elif isinstance(stdin,int):
          commandStdin = stdin
       else:
          try:
             stdinPath = str(stdin)
          except UnicodeEncodeError:
             try:
                stdinPath = unicode(stdin).encode('unicode_escape')
             except:
                stdinPath = None
          except:
             stdinPath = None
          if stdinPath:
             if os.path.isfile(stdinPath):
                try:
                   fpStdin = open(stdinPath,'r')
                except (IOError,OSError):
                   exitStatus = 1
                   errData.append("File %s could not be opened.\n" % (stdinPath))
                   if streamOutput:
                      sys.stderr.write("File %s could not be opened.\n" % (stdinPath))
                      sys.stderr.flush()
                else:
                   commandStdin = fpStdin
             else:
                exitStatus = 1
                errData.append("File %s does not exist.\n" % (stdinPath))
                if streamOutput:
                   sys.stderr.write("File %s does not exist.\n" % (stdinPath))
                   sys.stderr.flush()
          else:
             exitStatus = 1
             errData.append("Bad argument type specified for stdin.\n")
             if streamOutput:
                sys.stderr.write("Bad argument type specified for stdin.\n")
                sys.stderr.flush()

    if not exitStatus:
       BUFSIZ = 4096
       if isinstance(command,list):
          child = subprocess.Popen(command,bufsize=BUFSIZ, \
                                   stdin=commandStdin, \
                                   stdout=subprocess.PIPE, \
                                   stderr=subprocess.PIPE, \
                                   close_fds=True)
       else:
          try:
             commandStr = str(command)
          except UnicodeEncodeError:
             commandStr = unicode(command).encode('unicode_escape')
          commandArgs = shlex.split(commandStr)
          child = subprocess.Popen(commandArgs,bufsize=BUFSIZ, \
                                   stdin=commandStdin, \
                                   stdout=subprocess.PIPE, \
                                   stderr=subprocess.PIPE, \
                                   close_fds=True)
       commandPid = child.pid
       childout   = child.stdout
       childoutFd = childout.fileno()
       childerr   = child.stderr
       childerrFd = childerr.fileno()

       outEOF = False
       errEOF = False

       while True:
           toCheck = []
           if not outEOF:
               toCheck.append(childoutFd)
           if not errEOF:
               toCheck.append(childerrFd)
           try:
               readyRead,readyWrite,readyException = select.select(toCheck,[],[]) # wait for input
           except select.error,err:
               readyRead = []
               readyWrite = []
               readyException = []
           if childoutFd in readyRead:
               outChunk = os.read(childoutFd,BUFSIZ)
               if outChunk == '':
                   outEOF = True
               outData.append(outChunk)
               if streamOutput:
                   sys.stdout.write(outChunk)
                   sys.stdout.flush()

           if childerrFd in readyRead:
               errChunk = os.read(childerrFd,BUFSIZ)
               if errChunk == '':
                   errEOF = True
               errData.append(errChunk)
               if streamOutput:
                   sys.stderr.write(errChunk)
                   sys.stderr.flush()

           if outEOF and errEOF:
               break

       pid,exitStatus = os.waitpid(commandPid,0)
       commandPid = 0
       if fpStdin:
          try:
             fpStdin.close()
          except:
             pass

       try:
          signal.signal(signal.SIGINT,sig_INT_handler)
          signal.signal(signal.SIGHUP,sig_HUP_handler)
          signal.signal(signal.SIGTERM,sig_TERM_handler)
       except UnboundLocalError:
   #      happens when used in a thread
          pass
       except:
          print traceback.format_exc()

       if exitStatus != 0:
           if   os.WIFSIGNALED(exitStatus):
              sys.stderr.write("%s failed w/ signal %d\n" % (command,os.WTERMSIG(exitStatus)))
           else:
              if os.WIFEXITED(exitStatus):
                  exitStatus = os.WEXITSTATUS(exitStatus)
              sys.stderr.write("%s failed w/ exit code %d\n" % (command,exitStatus))
           if not streamOutput:
              sys.stderr.write("%s\n" % ("".join(errData)))

    return exitStatus,"".join(outData),"".join(errData)


def getCommandOutput(command,
                     streamOutput=False):
    exitStatus,stdOut,stdErr = executeCommand(command,
                                              streamOutput=streamOutput)

    return exitStatus,stdOut,stdErr


def getDriverNumber(driverFileName):
    driverNumRslt = re.search(r'[0-9]+',os.path.split(driverFileName)[1])
    if driverNumRslt == None:
        return None
    return driverNumRslt.group()


def writeFile(fileName,text):
    file_object = open(fileName, "w")
    if file_object:
        file_object.write(text)
        file_object.close()
    else:
        raise RuntimeError, 'could not open %s for writing' % (fileName)

