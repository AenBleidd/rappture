# ----------------------------------------------------------------------
#  EXAMPLE: UQ Example of Enabling and Disabling UQ Inputs
# ======================================================================
#  AUTHOR:  Martin Hunt, Purdue University
#  Copyright (c) 2015  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

import Rappture
import sys

# uncomment these for debugging
# sys.stderr = open('tool.err', 'w')
# sys.stdout = open('tool.out', 'w')

rx = Rappture.PyXml(sys.argv[1])

vsweep = rx['input.(vsweep).current'].value
temp = rx['input.(temperature).current'].value
length = rx['input.(length).current'].value

rx.close()
