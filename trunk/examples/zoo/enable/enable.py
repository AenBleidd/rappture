# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <enable> attributes
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
# sys.stdout = open('enc.out', 'w')
# sys.stderr = open('enc.err', 'w')

# open the XML file containing the run parameters
rx = Rappture.PyXml(sys.argv[1])

model = rx['input.(model).current'].value

if model == 'dd':
    result = "Drift-Diffusion:\n"
    recomb = rx['input.(dd).(recomb).current'].value
    result += "  Recombination model: %s\n" % recomb
    if recomb:
        taun = rx['input.(dd).(taun).current'].value
        taup = rx['input.(dd).(taup).current'].value
        result += "  TauN: %s\n" % taun
        result += "  TauP: %s\n" % taup
elif model == 'bte':
    result = "Boltzmann Transport Equation:\n"
    temp = rx["input.(bte).(temp).current"].value
    result += "  Temperature: %s\n" % temp
    secret = rx['input.(bte).(secret).current'].value
    result += "  Hidden number: %s\n" % secret
elif model == 'negf':
    result = "NEGF Analysis:\n"
    tbe = rx['input.(negf).(tbe).current'].value
    result += "  Tight-binding energy: %s\n" % tbe
    tau = rx['input.(negf).(tau).current'].value
    result += "  High-energy lifetime: %s\n" % tau

rx['output.log'] = result

# save the updated XML describing the run...
rx.close()
