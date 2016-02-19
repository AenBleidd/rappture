# ----------------------------------------------------------------------
#  EXAMPLE: Rappture Example of a Parallelepiped shape from PDB data.
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
# sys.stdout = open('group.out', 'w')
# sys.stderr = open('group.err', 'w')

# open the XML file containing the run parameters
rx = Rappture.PyXml(sys.argv[1])


pdb = """
ATOM      0   Si UNK A   1       0.000   0.000   0.000  1.00
ATOM      1   Si UNK A   1       2.710   2.710   0.000  1.00
ATOM      2   Si UNK A   1       2.710   0.000   2.710  1.00
ATOM      3   Si UNK A   1       0.000   2.710   2.710  1.00
ATOM      4   Si UNK A   1       5.420   2.710   2.710  1.00
ATOM      5   Si UNK A   1       2.710   5.420   2.710  1.00
ATOM      6   Si UNK A   1       2.710   2.710   5.420  1.00
ATOM      7   Si UNK A   1       5.420   5.420   5.420  1.00
ATOM      8   Si UNK A   1       1.355   1.355   1.355  1.00
TER       9      UNK A   1
"""

components = rx["output.structure.components"]
components['molecule.pdb'] = pdb

components['parallelepiped.origin'] = "0.0 0.0 0.0"
components['parallelepiped.scale'] = "5.42"
# components['parallelepiped.scale'] = "1.0"

components['parallelepiped.vector(1)'] = ".5 .5 .0"
components['parallelepiped.vector(2)'] = ".5 .0 .5"
components['parallelepiped.vector(3)'] = ".0 .5 .5"

rx.close()
