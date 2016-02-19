# ----------------------------------------------------------------------
#  EXAMPLE: Simple UQ Example of Curve Output
# ======================================================================
#  AUTHOR:  Martin Hunt, Purdue University
#  Copyright (c) 2015  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

import Rappture
import sys
import numpy as np

# uncomment these for debugging
# sys.stderr = open('tool.err', 'w')
# sys.stdout = open('tool.out', 'w')

rx = Rappture.PyXml(sys.argv[1])

vsweep = rx['input.(vsweep).current'].value
vsweep = float(Rappture.Units.convert(vsweep, to='V', units='off'))

# get 200 values between 0 and 10
xvals = np.linspace(0, 10, 200)


# our function for the curve
def f(x, v):
    return .1*x*x - (5 - v)**2

# evaluate our equation at the xvals
yvals = f(xvals, vsweep)

# save output as a curve
#rx['output.curve(f).about.label'] = 'Plot of f(x)'
rx['output.curve(f).component.xy'] = (xvals, yvals)


rx.close()
