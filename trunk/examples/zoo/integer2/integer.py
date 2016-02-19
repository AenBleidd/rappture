# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <integer> elements
# ======================================================================
#  AUTHOR:  Martin Hunt, Purdue University
#  Copyright (c) 2015  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

import Rappture
import sys

# open the XML file containing the run parameters
rx = Rappture.PyXml(sys.argv[1])

# read the inputs into a list
n = []
for i in range(1, 4):
    n.append(rx['input.(input%d).current' % i].value)

print 'n=', n

# build an output string
outstr = ''
for i in range(1, 4):
    outstr += "input%d = %s\n" % (i, n[i - 1])

# write the output string to the xml file
rx['output.string(out).current'] = outstr

# save the updated XML describing the run...
rx.close()
