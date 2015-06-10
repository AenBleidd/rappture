# ----------------------------------------------------------------------
#  GRAPH
#
#  This simple example shows how you can use the Rappture toolkit
#  to handle I/O for a simple simulator--in this case, one that
#  evaluates an x/y graph
#
# ======================================================================
#  AUTHOR:  Martin Hunt, Purdue University
#  Copyright (c) 2015  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

# Note: You will not see stdout and stderr when this
# tool is run by Rappture.  You can either run this tool from the command
# line by passing in a driver xml file like this:
# ~/rap/rappture/examples/graph> python graph.py driver1234.xml
#
# or you can redirect stdout and stderr to files by uncommenting the
# two lines after the import sys

import Rappture
import numpy as np
import sys

# uncomment these for debugging
# sys.stderr = open('graph.err', 'w')
# sys.stdout = open('graph.out', 'w')

io = Rappture.PyXml(sys.argv[1])

# When reading from xml, all values are strings
xmin = float(io['input.number(min).current'].value)
xmax = float(io['input.number(max).current'].value)
formula = io['input.string(formula).current'].value
print 'formula = %s' % formula

curve = io['output.curve(result)']
curve['about.label'] = 'Formula: Y vs X'
curve['yaxis.label'] = 'Y'
curve['xaxis.label'] = 'X'

num_points = 100
x = np.linspace(xmin, xmax, num_points)
y = eval(formula)
curve['component.xy'] = (x, y)

# Done. Write out the xml file for Rappture.
io.close()
