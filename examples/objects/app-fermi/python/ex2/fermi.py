#! /usr/bin/env python
# ----------------------------------------------------------------------
#  EXAMPLE: Fermi-Dirac function in Python.
#
#  This simple example shows how to use Rappture within a simulator
#  written in Python.
# ======================================================================
#  AUTHOR:  Derrick Kearney, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================


import sys
import os
import getopt

import Rappture
from math import *

from fermi_io import fermi_io

def help(argv=sys.argv):
    return """%(prog)s [-h]

     -h | --help       - print the help menu

     Examples:
       %(prog)s

""" % {'prog':os.path.basename(argv[0])}

def main(argv=None):

    # initialize the global interface
    Rappture.Interface(sys.argv,fermi_io)

    # check the global interface for errors
    if (Rappture.Interface.error() != 0) {
        # there were errors while setting up the interface
        # dump the traceback
        o = Rappture.Interface.outcome()
        print >>sys.stderr, "%s", o.context()
        print >>sys.stderr, "%s", o.remark()
        return (Rappture.Interface.error())
    }

    # connect variables to the interface
    # look in the global interface for an object named
    # "temperature", convert its value to Kelvin, and
    # store the value into the address of T.
    # look in the global interface for an object named
    # "Ef", convert its value to electron Volts and store
    # the value into the address of Ef
    # look in the global interface for an object named
    # factorsTable and set the variable result to
    # point to it.

    T = Rappture.Interface.connect(name="temperature",
                                   hints=["units=K"])
    Ef = Rappture.Interface.connect(name="Ef",
                                    hints=["units=eV"]);
    p1 = Rappture.Interface.connect(name="fdfPlot");
    p2 = Rappture.Interface.connect(name="fdfPlot");

    # check the global interface for errors
    if (Rappture.Interface.error() != 0) {
        # there were errors while retrieving input data values
        # dump the tracepack
        o = Rappture.Interface.outcome()
        print >>sys.stderr, "%s", o.context()
        print >>sys.stderr, "%s", o.remark()
        return (Rappture.Interface.error())
    }

    # do science calculations
    nPts = 200;
    EArr = list() # [nPts]
    fArr = list() # [nPts]

    kT = 8.61734e-5 * T
    Emin = Ef - (10*kT)
    Emax = Ef + (10*kT)

    dE = (1.0/nPts)*(Emax-Emin)

    E = Emin;
    for idx in xrange(nPts):
        E = E + dE
        f = 1.0/(1.0 + exp((E - Ef)/kT))
        fArr.append(f)
        EArr.append(E)
        progress = (int)((E-Emin)/(Emax-Emin)*100)
        Rappture.Utils.progress(progress,"Iterating")

    # set up the curves for the plot by using the add command
    # add <name> <xdata> <ydata> -format <fmt>
    #
    # to group curves on the same plot, just keep adding curves
    # to save space, X array values are compared between curves.
    # the the X arrays contain the same values, we only store
    # one version in the internal data table, otherwise a new
    # column is created for the array. for big arrays this may take
    # some time, we should benchmark to see if this can be done
    # efficiently.

    p1.add(name="fdfCurve1", xdata=fArr, ydata=EArr, format="g:o")

    p2.add(name="fdfCurve2", xdata=fArr, ydata=EArr, format="b-o")
    p2.add(name="fdfCurve3", xdata=fArr, ydata=EArr, format="p--")

    # close the global interface
    # signal to the graphical user interface that science
    # calculations are complete and to display the data
    # as described in the views
    Rappture.Interface.close()

    return 0

if __name__ == '__main__':
    sys.exit(main())
