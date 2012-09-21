# ----------------------------------------------------------------------
#  FERMI STATISTICS
#
#  This simple example shows how you can use the Rappture toolkit
#  to handle I/O for a simple simulator--in this case, one that
#  computes Fermi-Dirac statistics.
#
#  This part contains the Rappture interface for the simulator.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

import Rappture

#T = 300
#Ef = -5.5
#result = []

T = Rappture.number('input', id="temperature", units='K', default=300)
Ef = Rappture.number('input', id="Ef", units='eV', default=-5.5)

#result = Rappture.table('output.result') \
#           .column('Energy (eV)',units='eV') \
#           .column('Fermi-Dirac Factor')
#result = []
