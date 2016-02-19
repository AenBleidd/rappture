# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <group> attributes
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

re = rx['input.group.(models).(recomb).current'].value
tn = rx['input.group.(models).(tau).(taun).current'].value
tp = rx['input.group.(models).(tau).(taup).current'].value

temp = rx['input.group.(ambient).(temp).current'].value
lat = rx['input.group.(ambient).(loc).(lat).current'].value
lon = rx['input.group.(ambient).(loc).(long).current'].value

rx['output.log'] = """Models:
  Recombination: %s
  taun = %s
  taup = %s

Ambient:
  tempK = %s
  lat, long = (%s, %s)
""" % (re, tn, tp, temp, lat, lon)

# save the updated XML describing the run...
rx.close()
