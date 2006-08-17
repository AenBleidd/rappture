# ----------------------------------------------------------------------
#
# ======================================================================
#  AUTHOR:  Derrick Kearney, Purdue University
#  Copyright (c) 2005  Purdue Research Foundation, West Lafayette, IN
# ======================================================================

import sys, os, re

def getCommandOutput(command):
    childin,childout,childerr = os.popen3(command)
    data = childout.read()
    err = childerr.read()
    errcode = childout.close()
    if err:
        error = '%s->%s' % (command, err)
        raise RuntimeError, error
    return data

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
