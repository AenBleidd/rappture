# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <number> elements
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

# The example this is based on did a copy of the inputs
# to outputs, like below.  This is very efficient, but
# would not be a normal thing to do.
# rx.copy('output.number(outt)', 'input.(temperature)')
# rx.copy('output.number(outv)', 'input.(vsweep)')

temp = rx['input.(temperature).current'].value

# If we need a unitless float, then this is necessary.
# But in this example, we do not because Rappture accepts
# numbers with units for the value.
# temp = float(Rappture.Units.convert(temp, to='K', units='off'))

vsweep = rx['input.(vsweep).current'].value
# vsweep = float(Rappture.Units.convert(vsweep, to='V', units='off'))

rx['output.number(temperature).about.label'] = 'Ambient temperature'
rx['output.number(temperature).units'] = 'K'
rx['output.number(temperature).current'] = temp

rx['output.number(vsweep).about.label'] = 'Voltage Sweep'
rx['output.number(vsweep).units'] = 'V'
rx['output.number(vsweep).current'] = vsweep

rx.close()
