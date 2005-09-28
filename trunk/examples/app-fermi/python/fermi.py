# ----------------------------------------------------------------------
#  EXAMPLE: Fermi-Dirac function in Python.
#
#  This simple example shows how to use Rappture within a simulator
#  written in Python.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
import Rappture
import Rappture.Units
import sys
from math import *

# open the XML file containing the run parameters
driver = Rappture.library(sys.argv[1])

Tstr = driver.get('input.(temperature).current')
T = Rappture.Units.convert(Tstr, to="K", units="off")

Efstr = driver.get('input.(Ef).current')
Ef = Rappture.Units.convert(Efstr, to="eV", units="off")

kT = 8.61734e-5 * T
Emin = Ef - 10*kT
Emax = Ef + 10*kT

E = Emin
dE = 0.005*(Emax-Emin)
while E < Emax:
    f = 1.0/(1.0 + exp((E - Ef)/kT))
    line = "%g %g\n" % (f, E)
    driver.put('output.curve(f12).component.xy', line, append=1)
    E = E + dE

Rappture.result(driver)
sys.exit()
