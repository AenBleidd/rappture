# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <histogram> elements
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

rx = Rappture.PyXml(sys.argv[1])

num_points = int(rx['input.(points).current'].value)
x = np.linspace(1, 10, num_points)

# generate a single histogram
hist = rx['output.histogram(single)']
hist['about.label'] = "Single histogram"
hist['about.description'] = \
    "This is an example of a single histogram."
hist['xaxis.label'] = "Time"
hist['xaxis.description'] = "Time during the experiment."
hist['xaxis.units'] = "s"
hist['yaxis.label'] = "Voltage v(11)"
hist['yaxis.description'] = "Output from the amplifier."
hist['yaxis.units'] = "V"
y = np.cos(x)/(1+x)
hist['component.xy'] = (x, y)

# generate multiple histograms on the same plot
for factor in [1, 2]:
    hist = rx['output.histogram(multi%s)' % factor]
    hist['about.group'] = "Multiple histogram"
    hist['about.label'] = "Factor a=%s" % factor
    hist['about.description'] = \
        "This is an example of a multiple histograms on the same plot."
    hist['xaxis.label'] = "Frequency"
    hist['xaxis.description'] = "Frequency of the input source."
    hist['xaxis.units'] = "Hz"
    hist['yaxis.label'] = "Current"
    hist['yaxis.description'] = "Current through the pull-down resistor."
    hist['yaxis.units'] = "uA"
    hist['yaxis.log'] = "log"
    y = np.power(2.0, factor*x)/x
    hist['component.xy'] = (x, y)

# For the next part, only use 10 labels
x = np.linspace(1, 10, 10)

# generate a name value histogram
hist = rx['output.histogram(namevalue)']
hist['about.label'] = "Name value histogram"
hist['about.description'] = \
    "This is an example of a name value histogram."
hist['about.type'] = "scatter"
hist['xaxis.label'] = "Time"
hist['xaxis.description'] = "Time during the experiment."
hist['xaxis.units'] = "s"
hist['yaxis.label'] = "Voltage v(11)"
hist['yaxis.description'] = "Output from the amplifier."
hist['yaxis.units'] = "V"
y = np.cos(x)/(1+x)
labels = ['Bengal Tigers', 'Bears', 'Lions', 'Monkeys', 'Hawks',
    'Elephants', 'Foxes', 'Geckos', 'Zebras', 'Giraffes']
hist['component.xy'] = (labels, y)

hist = rx['output.histogram(width)']
hist['about.label'] = "Variable-Width histogram"
hist['about.description'] = \
    "This is an example of an <xhw> histogram."
hist['xaxis.label'] = "Time"
hist['xaxis.description'] = "Time during the experiment."
hist['xaxis.units'] = "s"
hist['yaxis.label'] = "Voltage v(11)"
hist['yaxis.description'] = "Output from the amplifier."
hist['yaxis.units'] = "V"
y = x
hist['component.xhw'] = (labels, y, x/10)

rx.close()
