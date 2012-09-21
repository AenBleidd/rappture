# ----------------------------------------------------------------------
#  GRAPH
#
#  This simple example shows how you can use the Rappture toolkit
#  to handle I/O for a simple simulator--in this case, one that
#  evaluates an x/y graph
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

import Rappture
import sys
from math import *

io = Rappture.library(sys.argv[1])

xmin = float(io.get('input.number(min).current'))
xmax = float(io.get('input.number(max).current'))
formula = io.get('input.string(formula).current')
print 'formula = %s' % formula
npts = 100

io.put('output.curve(result).about.label','Formula: Y vs X',append=0)
io.put('output.curve(result).yaxis.label','Y')
io.put('output.curve(result).xaxis.label','X')

for i in range(npts):
    x = (xmax-xmin)/npts * i + xmin;
    y = eval(formula)
    io.put('output.curve(result).component.xy', '%g %g\n' % (x,y), append=1)

Rappture.result(io)
