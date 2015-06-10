# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <number> elements
# ======================================================================
#  AUTHOR:  Martin Hunt, Purdue University
#  Copyright (c) 2015  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
import Rappture
import sys

# This tool takes three input numbers and prints
# their values in the output.

rx = Rappture.PyXml(sys.argv[1])

outstr = ''
for num in range(1, 4):
    val = rx['input.(input%d).current' % num].value
    outstr += 'input%d = %s\n' % (num, val)
rx['output.string(out).current'] = outstr

rx.close()
