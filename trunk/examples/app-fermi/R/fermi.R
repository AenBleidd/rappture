# ----------------------------------------------------------------------
#  EXAMPLE: Fermi-Dirac function in Tcl.
#
#  This simple example shows how to use Rappture within a simulator
#  written in Tcl.
# ======================================================================
#  AUTHOR:  Derrick Kearney, Purdue University
#  Copyright (c) 2005-2011  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

require(Rappture)

# save the command line arguments
args <- commandArgs();

# pick out the driver file which is the 7th element in the array
# our array looks like this:
# [1] "/usr/lib/R/bin/exec/R"
# [2] "--slave"
# [3] "--no-save"
# [4] "--no-restore"
# [5] "--silent"
# [6] "--args"
# [7] "driver.xml"

# open the XML file containing the run parameters
driver <- rp_lib(args[7])

T <- rp_lib_get_string(driver,"input.number(temperature).current")
T <- rp_units_convert_double(T,"K")
Ef <- rp_lib_get_string(driver,"input.number(Ef).current")
Ef <- rp_units_convert_double(Ef,"eV")

kT <- 8.61734e-5 * T
Emin <- Ef - 10*kT
Emax <- Ef + 10*kT

dE <- 0.005*(Emax-Emin)

y <- seq(Emin,Emax,by=dE)
x <- sapply(y,function(E) 1.0/(1.0 + exp((E - Ef)/kT)))

# Label output graph with title, x-axis label,
# y-axis lable, and y-axis units
rp_lib_put_string(driver,"output.curve(f12).about.label","Fermi-Dirac Factor",FALSE)
rp_lib_put_string(driver,"output.curve(f12).xaxis.label","Fermi-Dirac Factor",FALSE)
rp_lib_put_string(driver,"output.curve(f12).yaxis.label","Energy",FALSE)
rp_lib_put_string(driver,"output.curve(f12).yaxis.units","eV",FALSE)

# coerce our arrays into a string of the form:
# "x1 y1 \n x2 y2 \n x3 y3\n"...
# store the string in the Rappture library object
s <- paste(sprintf("%g %g\n",x,y),collapse="")
rp_lib_put_string(driver,"output.curve(f12).component.xy",s,FALSE)


# save the updated XML describing the run...
rp_lib_result(driver)
