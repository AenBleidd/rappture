# ----------------------------------------------------------------------
#  FERMI STATISTICS
#
#  This simple example shows how you can use the Rappture toolkit
#  to handle I/O for a simple simulator--in this case, one that
#  computes Fermi-Dirac statistics.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

from math import *
import sys
import Rappture

import fermi_io
from fermi_io import *

# do this right at the start, to handle command line args
Rappture.interface(sys.argv, fermi_io)

kT = 8.61734e-5 * T
Emin = Ef - 10*kT
Emax = Ef + 10*kT

E = Emin; dE = 0.005*(Emax-Emin)

Rappture.driver.put("output.curve(f12).about.label", "Fermi-Dirac Factor")
Rappture.driver.put("output.curve(f12).xaxis.label", "Energy")
Rappture.driver.put("output.curve(f12).xaxis.units", "eV")
path = "output.curve(f12).component.xy"

while E < Emax:
    f = 1.0/(1.0 + exp((E - Ef)/kT))
    # result.append( [E,f] )
    value = "%s  %s\n" % (f,E)
    Rappture.driver.put(path,value,append=1)
    E += dE

