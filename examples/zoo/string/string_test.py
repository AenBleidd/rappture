# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <string> elements
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

title = rx['input.(title).current'].value
indeck = rx['input.(indeck).current'].value

rx['output.string(outt).about.label'] = "Echo of title"
rx['output.string(outt).current'] = title
rx['output.string(outi).about.label'] = "Echo of input"
rx['output.string(outi).current'] = indeck

# save the updated XML describing the run...
rx.close()
