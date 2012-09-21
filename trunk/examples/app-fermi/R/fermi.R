# ----------------------------------------------------------------------
#  EXAMPLE: Fermi-Dirac function in R.
#
#  This simple example shows how to use Rappture within a simulator
#  written in R.
# ======================================================================
#  AUTHOR:  Derrick Kearney, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
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

rp_utils_progress(10,"reading inputs")

T <- rp_lib_get_string(driver,"input.number(temperature).current")
T <- rp_units_convert_double(T,"K")
Ef <- rp_lib_get_string(driver,"input.number(Ef).current")
Ef <- rp_units_convert_double(Ef,"eV")

rp_utils_progress(30,"performing calculations")

kT <- 8.61734e-5 * T
Emin <- Ef - 10*kT
Emax <- Ef + 10*kT

dE <- 0.005*(Emax-Emin)

rp_utils_progress(60,"performing calculations")

y <- seq(Emin,Emax,by=dE)
x <- sapply(y,function(E) 1.0/(1.0 + exp((E - Ef)/kT)))

rp_utils_progress(80,"storing results")

# Label output graph with title, x-axis label,
# y-axis lable, and y-axis units
rp_lib_put_string(driver,"output.curve(f12).about.label","Fermi-Dirac Factor",FALSE)
rp_lib_put_string(driver,"output.curve(f12).xaxis.label","Fermi-Dirac Factor",FALSE)
rp_lib_put_string(driver,"output.curve(f12).yaxis.label","Energy",FALSE)
rp_lib_put_string(driver,"output.curve(f12).yaxis.units","eV",FALSE)

rp_utils_progress(90,"storing results")

# coerce our arrays into a string of the form:
# "x1 y1 \n x2 y2 \n x3 y3\n"...
# store the string in the Rappture library object
s <- paste(sprintf("%g %g\n",x,y),collapse="")
rp_lib_put_string(driver,"output.curve(f12).component.xy",s,FALSE)


rp_utils_progress(100,"preparing graphs")

# save the updated XML describing the run...
rp_lib_result(driver)
