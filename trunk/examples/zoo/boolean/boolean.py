# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <boolean> elements
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
# sys.stdout = open('boolean.out', 'w')
# sys.stderr = open('boolean.err', 'w')

# open the XML file containing the run parameters
rx = Rappture.PyXml(sys.argv[1])

choice = rx['input.(iimodel).current'].value
rx['output.boolean(outb).about.label'] = "Echo of boolean value iimodel"
rx['output.boolean(outb).current'] = choice

choice1 = rx['input.(iimodel1).current'].value
rx['output.boolean(outb1).about.label'] = "Echo of boolean value iimodel1"
rx['output.boolean(outb1).current'] = choice1

choice2 = rx['input.(iimodel2).current'].value
rx['output.boolean(outb2).about.label'] = "Echo of boolean value iimodel2"
rx['output.boolean(outb2).current'] = choice2

choice3 = rx['input.(iimodel3).current'].value
rx['output.boolean(outb3).about.label'] = "Echo of boolean value iimodel3"
rx['output.boolean(outb3).current'] = choice3

# save the updated XML describing the run...
rx.close()
