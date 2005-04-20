# ----------------------------------------------------------------------
#  FERMI STATISTICS
#
#  This simple example shows how you can use the Rappture toolkit
#  to handle I/O for a simple simulator--in this case, one that
#  computes Fermi-Dirac statistics.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2005  Purdue Research Foundation, West Lafayette, IN
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

while E < Emax:
    f = 1.0/(1.0 + exp((E - Ef)/kT))
    result.append( [E,f] )
    E += dE
