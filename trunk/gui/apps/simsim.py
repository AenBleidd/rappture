#!/usr/bin/env python

import sys
import os
import Rappture
import random
import getopt
import time
import re


def defaultHandler(child):
    childchildList = None
    defaultList = child.children(type="default")
    if (defaultList != []):
        value = defaultList[0].get()
        if (value == ""):
            childchildList = child.children()
        else:
            child.put("current",str(value))
    else :
        if (child.element(as="type") != "components"):
            childchildList = child.children()
    return childchildList

def numberHandler(child):
    value = ""

    units = child.get("units") or ""
    min = child.get("min") or ""
    max = child.get("max") or ""

    if ((min != "")):
        if (units != ""):
            min = Rappture.Units.convert(min,units,"off")
        else:
            min = float(min)

    if (max != ""):
        if (units != ""):
            max = Rappture.Units.convert(max,units,"off")
        else:
            max = float(max)

    if ((min != "") and (max != "")):
        value = random.uniform(min,max)
        if (units != ""):
            value = Rappture.Units.convert(str(value),units,"on")
        child.put("current",str(value))
    else:
        defaultHandler(child)

def integerHandler(child):
    value = ""

    units = child.get("units") or ""
    min = child.get("min") or ""
    max = child.get("max") or ""

    if ((min != "")):
        if (units != ""):
            min = Rappture.Units.convert(min,units,"off")
        else:
            min = int(min)

    if (max != ""):
        if (units != ""):
            max = Rappture.Units.convert(max,units,"off")
        else:
            max = int(max)

    if ((min != "") and (max != "")):
        value = random.randint(min,max)
        #if (units != ""):
        #    value = Rappture.Units.convert(value,units,"on")
        child.put("current",str(value))
    else:
        defaultHandler(child)

def booleanHandler(child):
    value = random.randint(0,1)
    if (value == 1):
        value = "yes"
    else:
        value = "no"
    child.put("current",str(value))

def writeDriver(lib):
    driverFile = 'driver%d.xml' % time.time()
    fp = open(driverFile,'w')
    fp.write(lib.xml())
    fp.close()
    return driverFile

def printHelp():
    print """%s [-t <path> | -x | -d | -h]

      -t | --tool <path>           - use the tool.xml file specified at <path>
      -x | --driver-only           - only produce the driver file
      -d | --use-defaults          - use default values instead of generating
                                     random values based on min/max values for
                                     number and integer elements.
      -h | --help                  - print this help menu

""" % (sys.argv[0])
    sys.exit()


appStartTime = time.time()

tool = "tool.xml"
driverOnly = False
defaults = False

longOpts = ["tool=","use-defaults","help"]
shortOpts = "t:dh"
opts, args = getopt.getopt(sys.argv[1:], shortOpts, longOpts)

for o, v in opts:
    if o in ("-t", "--tool"):
        tool = v
    elif o in ("-x", "--driver-only"):
        driverOnly = True
    elif o in ("-d", "--use-defaults"):
        defaults = True
    elif o in ("-h", "--help"):
        printHelp()

if (not os.path.exists(tool)):
    print "\ntool file \"%s\" does not exists, use -t\n" % (tool)
    printHelp()

lib = Rappture.library(tool)

childList = lib.children("input")

i = 0
while i < len(childList):
    child = childList[i]
    i = i + 1
    type = child.element(as="type")
    if ((type == "number") and (not defaults)):
        numberHandler(child)
    elif ((type == "integer") and (not defaults)):
        integerHandler(child)
    elif ((type == "boolean") and (not defaults)):
        booleanHandler(child)
    else:
        addList = defaultHandler(child)
        if addList != None:
            childList = childList + addList

for pathValue in args:
    (path,value) = pathValue.split(":")
    lib.put(path+".current",value)

driverFile = writeDriver(lib)
if driverOnly:
    print driverFile
    sys.exit()

toolDir = os.path.dirname(tool)
toolCommand = lib.get("tool.command")
toolCommand = re.sub("\@driver",driverFile,toolCommand)
toolCommand = re.sub("\@tool",toolDir,toolCommand)

commandStartTime = time.time()
time.clock()
output = Rappture.tools.getCommandOutput(toolCommand)
cputime = time.clock()
commandStopTime = time.time()

job = 1
startTime =  commandStartTime - appStartTime
walltime = commandStopTime - commandStartTime
status = 0

print output
print "MiddlewareTime: job=%d event=simulation start=%f walltime=%f cputime=%f status=%d"\
    % (job,startTime,walltime,cputime,status)
