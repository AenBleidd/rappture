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

n = rx['input.(points).current'].value
rx['output.integer(outn).about.label'] = "Echo of points"
rx['output.integer(outn).current'] = n

# save the updated XML describing the run...
rx.close()
