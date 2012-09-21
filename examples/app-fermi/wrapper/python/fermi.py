# ----------------------------------------------------------------------
#  EXAMPLE: Fermi-Dirac function in Python.
#
#  This simple example shows how to use Rappture to wrap up a
#  simulator written in Matlab or some other language.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
import Rappture
import sys, os, commands, string

# open the XML file containing the run parameters
driver = Rappture.library(sys.argv[1])

Tstr = driver.get('input.(temperature).current')
T = Rappture.Units.convert(Tstr, to="k", units="off")

Efstr = driver.get('input.(Ef).current')
Ef = Rappture.Units.convert(Efstr, to="eV", units="off")

fid = open('indeck','w')
infile = "%(Ef)s\n%(T)s\n" % { 'Ef':Ef, 'T':T }
fid.write(infile)
fid.close()

try:
  out = commands.getoutput('octave --silent fermi.m < indeck')
except:
  sys.stderr.write('Error during execution of fermi.m')
  exit(1);

driver.put('output.log',out)

fid = open('out.dat','r')
info = fid.readlines()
fid.close()

# Label output graph with title, x-axis label,
# y-axis lable, and y-axis units
driver.put('output.curve(f12).about.label','Fermi-Dirac Factor')
driver.put('output.curve(f12).xaxis.label','Fermi-Dirac Factor')
driver.put('output.curve(f12).yaxis.label','Energy')
driver.put('output.curve(f12).yaxis.units','eV')

# skip over the first 4 header lines
for line in info[6:]:
    f,E = string.split(line[:-1])
    f,E = float(f),float(E)
    xy = "%g %g\n" % (f,E)
    driver.put('output.curve(f12).component.xy',xy,append=1)

os.remove('indeck'); os.remove('out.dat')

Rappture.result(driver)
sys.exit(0)
