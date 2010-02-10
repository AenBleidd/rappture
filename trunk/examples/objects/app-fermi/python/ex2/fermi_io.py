#! /usr/bin/env python

import sys
import os
import getopt

def fermi_io():

    # assume global interface variable.
    # newly created objects are stored in the
    # global interface variable. if there is more
    # than one global interface variable, the
    # interface that should be used is specified
    # using the Rp_???InitX version of the funtion
    # where ??? represents the type of object the
    # user is trying to create.

    # describe the inputs
    # declare a number named "temperature",
    # with units of Kelvin, default value 300,
    #
    # the next number is named "Ef", has units of
    # electron Volts, default value of -5.5
    #
    # Rappture.Number is assumed to be an input
    Rappture.Number(name="temperature",units="K",default=300)
    Rappture.Number(name="Ef",units="eV",default=-5.5)

    # Most simple xy plots for output
    # Because it is a single plot, it gets its own view
    # The plot is placed in the position 1,1 of the view
    plot = Rappture.Plot(name="fdfPlot")
    plot.xaxis(label="Fermi-Dirac Factor",
        desc="Plot of Fermi-Dirac Calculation")
    plot.yaxis(label="Energy",
        desc="Energy cooresponding to each fdf",
        hints=["units=eV"])

    # Declaring a second plot creates a new view
    # The new plot will be placed in the position 1,1 of its view
    # User can do simple plot grouping using the add function
    # in the science code.
    Rappture.Plot(name="fdfPlot",
        xlabel="Fermi-Dirac Factor",
        xdesc="Plot of Fermi-Dirac Calculation",
        yaxis="Energy",
        ydesc="Energy cooresponding to each fdf",
        yhints=["units=eV"])

def help(argv=sys.argv):
    return """%(prog)s [-h]

     -h | --help       - print the help menu

     Examples:
       %(prog)s

""" % {'prog':os.path.basename(argv[0])}

def main(argv=None):

    if argv is None:
        argv = sys.argv

    if len(argv) < 1:
        msg = "%(prog)s requires at least 0 argument" % {'prog':argv[0]}
        print >>sys.stderr, msg
        print >>sys.stderr, help()
        return 2

    longOpts = ["help"]
    shortOpts = "h"
    try:
        opts,args = getopt.getopt(argv[1:],shortOpts,longOpts)
    except getopt.GetOptError, msg:
        print >>sys.stderr, msg
        print >>sys.stderr, help()
        return 2

if __name__ == '__main__':
    sys.exit(main())

