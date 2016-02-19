# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <loader> elements
# ======================================================================
#  AUTHOR:  Miartin Hunt, Purdue University
#  Copyright (c) 2015  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

import Rappture
import sys

# open the XML file containing the run parameters
rx = Rappture.PyXml(sys.argv[1])

one = rx['input.(one).current'].value
two = rx['input.(two).current'].value

rx['output.log'] = "Input #1: %s\nInput #2: %s" % (one, two)

# save the updated XML describing the run...
rx.close()
