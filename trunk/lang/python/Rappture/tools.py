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
import select
import signal

commandPid = 0

def sig_handler(signalType, frame):
    global commandPid
    if commandPid:
        os.kill(commandPid,signal.SIGTERM)


def getCommandOutput(command,
                     streamOutput=False):
    global commandPid

    sig_INT_handler = signal.signal(signal.SIGINT,sig_handler)
    sig_HUP_handler = signal.signal(signal.SIGHUP,sig_handler)
    sig_TERM_handler = signal.signal(signal.SIGTERM,sig_handler)

    BUFSIZ = 4096
    if isinstance(command,list):
       child = subprocess.Popen(command,bufsize=BUFSIZ, \
                                stdout=subprocess.PIPE, \
                                stderr=subprocess.PIPE, \
                                close_fds=True)
    else:
       child = subprocess.Popen(command,shell=True,bufsize=BUFSIZ, \
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

    outData = []
    errData = []

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

    pid,err = os.waitpid(commandPid,0)
    commandPid = 0

    signal.signal(signal.SIGINT,sig_INT_handler)
    signal.signal(signal.SIGHUP,sig_HUP_handler)
    signal.signal(signal.SIGTERM,sig_TERM_handler)

    if err != 0:
        if   os.WIFSIGNALED(err):
           sys.stderr.write("%s failed w/ signal %d\n" % (command,os.WTERMSIG(err)))
        else:
           if os.WIFEXITED(err):
               err = os.WEXITSTATUS(err)
           sys.stderr.write("%s failed w/ exit code %d\n" % (command,err))
        if not streamOutput:
           sys.stderr.write("%s\n" % ("".join(errData)))

    return err,"".join(outData),"".join(errData)


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
