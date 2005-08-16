# ----------------------------------------------------------------------
#  COMPONENT: result - provides a simple way to report results
#
#  This routine encapsulates the action of reporting results
#  back to Rappture.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2005  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
import time

def result(lib):
    """
    Use this function to report the result of a Rappture simulation.
    Pass in the XML object representing the initial driver with
    results substituted into it.
    """

    runfile = 'run%d.xml' % time.time()
    fp = open(runfile,'w')
    fp.write(lib.xml())
    fp.close()

    # pass the name of the run file back to Rappture
    print '=RAPPTURE-RUN=>%s' % runfile
