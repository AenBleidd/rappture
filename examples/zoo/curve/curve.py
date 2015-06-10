# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <curve> elements
# ======================================================================
#  AUTHOR:  Martin Hunt, Purdue University
#  Copyright (c) 2015  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
import Rappture
import numpy as np
import sys

# open the XML file containing the run parameters
rx = Rappture.PyXml(sys.argv[1])

npts = int(rx['input.(points).current'].value)

# generate a single curve
single = rx['output.curve(single)']
single['about.label'] = "Single Curve"
single['about.description'] = 'This is an example of a single curve.'
single['xaxis.label'] = "Time"
single['xaxis.description'] = "Time during the experiment."
single['xaxis.units'] = "s"
single['yaxis.label'] = "Voltage v(11)"
single['yaxis.description'] = "Output from the amplifier."
single['yaxis.units'] = "V"

# generate curve
xmin, xmax = 0.01, 10.0
x = np.linspace(xmin, xmax, npts)
y = np.cos(x)/(1+x)

# plot it
single['component.xy'] = (x, y)

# These are also acceptable.
# single['component.xy'] = [x, y]
# single['component.xy'] = np.row_stack((x, y))

# generate multiple curves on the same plot
for factor in [1, 2]:
    curve = rx['output.curve(multi%s)' % factor]
    curve['about.group'] = "Multiple curve"
    curve['about.label'] = "Factor a=$factor"
    curve['about.description'] = \
        "This is an example of a multiple curves on the same plot."
    curve['xaxis.label'] = "Frequency"
    curve['xaxis.description'] = \
        "Frequency of the input source."
    curve['xaxis.units'] = "Hz"
    # curve['xaxis.scale'] = "log"
    curve['yaxis.label'] = "Current"
    curve['yaxis.description'] = \
        "Current through the pull-down resistor."
    curve['yaxis.units'] = "uA"
    curve['yaxis.log'] = "log"
    y = np.power(2.0, factor*x)/x
    curve['component.xy'] = (x, y)

# generate a scatter curve
curve = rx['output.curve(scatter)']
curve['about.label'] = "Scatter curve"
curve['about.description'] = \
    "This is an example of a scatter curve."
curve['about.type'] = "scatter"
curve['xaxis.label'] = "Time"
curve['xaxis.description'] = "Time during the experiment."
curve['xaxis.units'] = "s"
curve['yaxis.label'] = "Voltage v(11)"
curve['yaxis.description'] = "Output from the amplifier."
curve['yaxis.units'] = "V"

y = np.cos(x)/(1+x)
curve['component.xy'] = (x, y)

# generate a bar curve
curve = rx['output.curve(bars)']
curve['about.label'] = "Bar chart"
curve['about.description'] = \
    "This is an example of a scatter curve."
curve['about.type'] = "bar"
curve['xaxis.label'] = "Time"
curve['xaxis.description'] = "Time during the experiment."
curve['xaxis.units'] = "s"
curve['yaxis.label'] = "Voltage v(11)"
curve['yaxis.description'] = "Output from the amplifier."
curve['yaxis.units'] = "V"

x = np.arange(0, npts)
y = np.sin(x)/(1+x)
curve['component.xy'] = (x, y)

# generate mixed curves on the same plot
deg2rad = 0.017453292519943295

curve = rx['output.curve(line)']
curve['about.group'] = "Mixed element types"
curve['about.label'] = "Sine"
curve['about.description'] = \
    "This is an example of a mixed curves on the same plot."
curve['xaxis.label'] = "Degrees"
# curve['yaxis.label'] = "Sine"
curve['about.type'] = "line"

x = np.arange(0, 361, 30)
y = np.sin(x * deg2rad)
curve['component.xy'] = (x, y)

curve = rx['output.curve(bar)']
curve['about.group'] = "Mixed element types"
curve['about.label'] = "Cosine"
curve['about.description'] = \
    "This is an example of a mixed curves on the same plot."
curve['xaxis.label'] = "Degrees"
# curve['yaxis.label'] = "Cosine"
curve['about.type'] = "bar"
curve['about.style'] = "-barwidth 24.0"

y = np.cos(x * deg2rad)
curve['component.xy'] = (x, y)

curve = rx['output.curve(point)']
curve['about.group'] = "Mixed element types"
curve['about.label'] = "Random"
curve['about.description'] = \
    "This is an example of a mixed curves on the same plot."
curve['xaxis.label'] = "Degrees"
# curve['yaxis.label'] = "Random"
curve['about.type'] = "scatter"

x = np.arange(0, 361, 10)
y = np.random.rand(len(x)) * 2.0 - 1
curve['component.xy'] = (x, y)

# save the updated XML describing the run...
rx.close()
