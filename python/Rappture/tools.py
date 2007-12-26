# ----------------------------------------------------------------------
#
# ======================================================================
#  AUTHOR:  Derrick S. Kearney, Purdue University
#  AUTHOR:  Steve Clark, Purdue University
#  Copyright (c) 2005-2007  Purdue Research Foundation, West Lafayette, IN
# ======================================================================

import sys, os, re, popen2

# getCommandOutput function written by Steve Clark

def getCommandOutput(command,
                     streamOutput=False):
    global commandPid

    child = popen2.Popen3(command,1)
    commandPid = child.pid
    child.tochild.close() # don't need to talk to child
    childout = child.fromchild
    childoutFd = childout.fileno()
    childerr = child.childerr
    childerrFd = childerr.fileno()

    outEOF = errEOF = 0
    BUFSIZ = 4096

    outData = []
    errData = []

    while 1:
        toCheck = []
        if not outEOF:
            toCheck.append(childoutFd)
        if not errEOF:
            toCheck.append(childerrFd)
        ready = select.select(toCheck,[],[]) # wait for input
        if childoutFd in ready[0]:
            outChunk = os.read(childoutFd,BUFSIZ)
            if outChunk == '':
                outEOF = 1
            outData.append(outChunk)
            if streamOutput:
                sys.stdout.write(outChunk)
                sys.stdout.flush()

        if childerrFd in ready[0]:
            errChunk = os.read(childerrFd,BUFSIZ)
            if errChunk == '':
                errEOF = 1
            errData.append(errChunk)
            if streamOutput:
                sys.stderr.write(errChunk)
                sys.stderr.flush()

        if outEOF and errEOF:
            break

    err = child.wait()
    commandPid = 0
    if err != 0:
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
