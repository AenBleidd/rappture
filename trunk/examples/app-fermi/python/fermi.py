# ----------------------------------------------------------------------
#  EXAMPLE: Fermi-Dirac function in Python.
#
#  This simple example shows how to use Rappture within a simulator
#  written in Python.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#           Martin Hunt, Purdue University
#  Copyright (c) 2004-2015  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
import Rappture
import sys
import numpy as np

# Uncomment these lines to redirect
# python output and errors to files
# for easier debugging.
# sys.stderr = open('fermi.err', 'w')
# sys.stdout = open('fermi.out', 'w')

# open the XML file containing the run parameters
rx = Rappture.PyXml(sys.argv[1])

temp_str = rx['input.(temperature).current'].value
temp = Rappture.Units.convert(temp_str, to='K', units='off')

ef_str = rx['input.(Ef).current'].value
ef = Rappture.Units.convert(ef_str, to='eV', units='off')

kt = 8.61734e-5 * temp
emin = ef - 10*kt
emax = ef + 10*kt

# Label the output graph with a title, x-axis label,
# y-axis label, and y-axis units
f12 = rx['output.curve(f12)']  # a shortcut to save typing
f12['about.label'] = 'Fermi-Dirac Factor'
f12['xaxis.label'] = 'Fermi-Dirac Factor'
f12['yaxis.label'] = 'Energy'
f12['yaxis.units'] = 'eV'

# How many points to use to define our curve
numpts = 200

# The normal python approach would be to simply do this:
# energy = np.linspace(emin, emax, numpts)
# f = 1.0/(1.0 + np.exp((energy - ef)/kt))
# f12['component.xy'] = (f, energy)
# rx.close()

# But we want to show how to use the progress bar,
# so lets do things slowly and iteratively...

import time
energy = []
fermi = []
for i, e in enumerate(np.linspace(emin, emax, numpts)):
    f = 1.0/(1.0 + np.exp((e - ef)/kt))
    energy.append(e)
    fermi.append(f)
    Rappture.Utils.progress(i*100.0/numpts, "Iterating")
    time.sleep(0.01)
f12['component.xy'] = (fermi, energy)
rx.close()
