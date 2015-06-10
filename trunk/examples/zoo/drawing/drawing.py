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

value = rx['input.choice(analysis).current'].value
out = "Analysis Choice:  %s\n" % value

value = rx['input.number(trapezoid_top).current'].value
out += "Trapezoid Top:  %s\n" % value

value = rx['input.number(substrate_length).current'].value
out += "Substrate Length:  %s\n" % value

value = rx['input.number(feature_length).current'].value
out += "Feature Length:  %s\n" % value

value = rx['input.number(feature_height).current'].value
out += "Feature Height:  %s\n" % value

value = rx['input.string(indeck).current'].value
out += "String:  %s\n" % value

rx['output.string(out).about.label'] = "Echo of inputs"
rx['output.string(out).current'] = out

# save the updated XML describing the run...
rx.close()
