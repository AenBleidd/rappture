# ----------------------------------------------------------------------
#
# ======================================================================
#  AUTHOR:  Derrick Kearney, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
# ======================================================================

import sys
import signal

qID = []
queue_type = ''

def sigGEN_handler(signal, frame):
    global qID
    global queue_type
    if qID :
        for id in xrange(len(qID)):
            sys.stdout.write("Killing job with qID = :%s:\n" % (qID[id]))
            sys.stdout.flush()
            if queue_type == 'PBS':
                rmCmdOut = getCommandOutput('qdel %s' % (qID[id]))
            elif queue_type == 'Condor':
                rmCmdOut = getCommandOutput('condor_rm %s' % (qID[id]))
            else:
                # unrecognized queue_type
                break
            sys.stdout.write("\n%s\n" % rmCmdOut)
            sys.stdout.flush()
            time.sleep(10)
    sys.stdout.write("\nEXITING...\n")
    sys.stdout.flush()
    sys.exit(-1)


def sigINT_handler(signal, frame):
    sys.stdout.write('Received SIGINT!\n')
    sys.stdout.flush()
    sigGEN_handler(signal, frame)

def sigHUP_handler(signal, frame):
    sys.stdout.write('Received SIGHUP!\n')
    sys.stdout.flush()
    sigGEN_handler(signal, frame)

def sigQUIT_handler(signal, frame):
    sys.stdout.write('Received SIGQUIT!\n')
    sys.stdout.flush()
    sigGEN_handler(signal, frame)

def sigABRT_handler(signal, frame):
    sys.stdout.write('Received SIGABRT!\n')
    sys.stdout.flush()
    sigGEN_handler(signal, frame)

def sigTERM_handler(signal, frame):
    sys.stdout.write('Received SIGTERM!\n')
    sys.stdout.flush()
    sigGEN_handler(signal, frame)

def sigSTOP_handler(signal, frame):
    sys.stdout.write('Received SIGSTOP!\n')
    sys.stdout.flush()
    sigGEN_handler(signal, frame)

def sigKILL_handler(signal, frame):
    sys.stdout.write('Received SIGKILL!\n')
    sys.stdout.flush()
    sigGEN_handler(signal, frame)

signal.signal(signal.SIGINT, sigINT_handler)
signal.signal(signal.SIGHUP, sigHUP_handler)
signal.signal(signal.SIGQUIT, sigQUIT_handler)
signal.signal(signal.SIGABRT, sigABRT_handler)
signal.signal(signal.SIGTERM, sigTERM_handler)
#signal.signal(signal.SIGSTOP, sigSTOP_handler)
#signal.signal(signal.SIGKILL, sigKILL_handler)

