#
# Fermi-Dirac function in Ruby
#
# This example shows how to use Rappture within a simulator written in Ruby.
# =============================================================================
# Author: Benjamin Haley, Purdue University
# Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
# See the file "license.terms" for information on usage and 
# redistributioin of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# =============================================================================
# Takes one argument: driver (tool.xml)

# Load Rappture
require "Rappture"

# Create an instance of the Rappture I/O library;
# Pass the driver file name as an argument
rp = Rappture.new(ARGV[0])

# Get the input temperature
strT = rp.get("input.(temperature).current")
numT = rp.convert(strT, "K", Rappture::UNITS_OFF)

# Get the input Fermi energy
strEf = rp.get("input.(Ef).current")
numEf = rp.convert(strEf, "eV", Rappture::UNITS_OFF)

# Set the energy range and step size
kT = 8.61734e-5*numT
minE = numEf - 10.0*kT
maxE = numEf + 10.0*kT
currE = minE
dE = 0.005*(maxE - minE)

# Set information about the output plot
rp.put("output.curve(f12).about.label", "Fermi-Dirac Factor", Rappture::OVERWRITE)
rp.put("output.curve(f12).xaxis.label", "Fermi-Dirac Factor", Rappture::OVERWRITE)
rp.put("output.curve(f12).yaxis.label", "Energy", Rappture::OVERWRITE)
rp.put("output.curve(f12).yaxis.units", "eV", Rappture::OVERWRITE)

# Calculate the Fermi-Dirac function over the energy range, 
# and add the results to the output plot.
while currE < maxE
   f = 1.0/(1.0 + Math.exp((currE - numEf)/kT))
   currXY = sprintf("%g  %g\n", f, currE)
   rp.progress(((currE-minE)/(maxE-minE)*100.0), "Iterating")
   rp.put("output.curve(f12).component.xy", currXY, Rappture::APPEND)
   currE += dE
end

# Write the final results
rp.result(0)

