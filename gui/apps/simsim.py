#!/usr/bin/env python

import sys
import Rappture
import random
import getopt
import time


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
    print """simsim [-t <path> | -d | -h]

      -t | --tool <path>           - use the tool.xml file specified at <path>
      -d | --use-defaults          - use default values instead of generating
                                     random values based on min/max values for
                                     number and integer elements.
      -h | --help                  - print this help menu

"""
    sys.exit()


tool = "tool.xml"
defaults = False

longOpts = ["tool=","use-defaults","help"]
shortOpts = "t:dh"
opts, args = getopt.getopt(sys.argv[1:], shortOpts, longOpts)

for o, v in opts:
    if o in ("-t", "--tool"):
        tool = v
    elif o in ("-d", "--use-defaults"):
        defaults = True
    elif o in ("-h", "--help"):
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

print writeDriver(lib)
