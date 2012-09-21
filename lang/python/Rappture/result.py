# ----------------------------------------------------------------------
#  COMPONENT: result - provides a simple way to report results
#
#  This routine encapsulates the action of reporting results
#  back to Rappture.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
import time

def result(lib):
    """
    Use this function to report the result of a Rappture simulation.
    Pass in the XML object representing the initial driver with
    results substituted into it.
    """

    lib.put("tool.version.rappture.language", "python")
    lib.result()
