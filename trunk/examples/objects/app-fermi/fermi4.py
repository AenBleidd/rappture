#! /usr/bin/env python
# ----------------------------------------------------------------------
#  EXAMPLE: Fermi-Dirac function in Python.
#
#  This simple example shows how to use Rappture within a simulator
#  written in Python.
# ======================================================================
#  AUTHOR:  Derrick Kearney, Purdue University
#  Copyright (c) 2005-2009  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================


import sys
import os
import getopt

import Rappture
from math import *


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



    # create a rappture library from the file filePath
    lib = Rappture.Library(argv[1])

    nPts = 200;
    EArr = list() # [nPts];
    fArr = list() # [nPts];

    if (lib.error() != 0) {
        # cannot open file or out of memory
        print >>sys.stderr, lib.traceback()
        exit(lib.error());
    }

    T = Rappture.connect(lib,"temperature");
    Ef = lib.value("Ef","units=eV");

    if (lib.error() != 0) {
        # there were errors while retrieving input data values
        # dump the tracepack
        print >>sys.stderr, lib.traceback()
        exit(lib.error());
    }

    kT = 8.61734e-5 * T.value("K");
    Emin = Ef - 10*kT;
    Emax = Ef + 10*kT;

    dE = (1.0/nPts)*(Emax-Emin);

    E = Emin;
    for (size_t idx = 0; idx < nPts; idx++) {
        E = E + dE;
        f = 1.0/(1.0 + exp((E - Ef)/kT));
        fArr.append(f);
        EArr.append(E);
        Rappture.Utils.progress((int)((E-Emin)/(Emax-Emin)*100),"Iterating");
    }

    curveLabel = "Fermi-Dirac Curve"
    curveDesc = "Plot of Fermi-Dirac Calculation";

    # do it the easy way,
    # create a plot to add to the library
    # plot is registered with lib upon object creation
    # p1->add(nPts,xArr,yArr,format,curveLabel,curveDesc);

    p1 = Rappture.Plot(lib);
    p1.add(nPts,fArr,EArr,"",curveLabel,curveDesc);
    p1.propstr("xlabel","Fermi-Dirac Factor");
    p1.propstr("ylabel","Energy");
    p1.propstr("yunits","eV");

    lib.result();

    return 0;
}


if __name__ == '__main__':
    sys.exit(main())

