// ----------------------------------------------------------------------
//  EXAMPLE: Fermi-Dirac function in C++.
//
//  This simple example shows how to use Rappture within a simulator
//  written in C++.
// ======================================================================
//  AUTHOR:  Derrick Kearney, Purdue University
//  Copyright (c) 2005-2009  Purdue Research Foundation
//
//  See the file "license.terms" for information on usage and
//  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
// ======================================================================

#include "rappture.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>

int main(int argc, char * argv[]) {

    // create a rappture library from the file filePath
    Rappture::Library lib(argv[1]);
    Rappture::Number *T;

    double Ef           = 0.0;
    double E            = 0.0;
    double dE           = 0.0;
    double kT           = 0.0;
    double Emin         = 0.0;
    double Emax         = 0.0;
    double f            = 0.0;
    size_t nPts         = 200;
    double EArr[nPts];
    double fArr[nPts];

    if (lib.error() != 0) {
        // cannot open file or out of memory
        Rappture::Outcome o = lib.outcome();
        fprintf(stderr, "%s",o.context());
        fprintf(stderr, "%s",o.remark());
        exit(lib.error());
    }

    Rappture::connect(&lib,"temperature",T);
    lib.value("Ef", &Ef, 1, "units=eV");

    if (lib.error() != 0) {
        // there were errors while retrieving input data values
        // dump the tracepack
        Rappture::Outcome o = lib.outcome();
        fprintf(stderr, "%s",o.context());
        fprintf(stderr, "%s",o.remark());
        exit(lib.error());
    }

    kT = 8.61734e-5 * T->value("K");
    Emin = Ef - 10*kT;
    Emax = Ef + 10*kT;

    dE = (1.0/nPts)*(Emax-Emin);

    E = Emin;
    for (size_t idx = 0; idx < nPts; idx++) {
        E = E + dE;
        f = 1.0/(1.0 + exp((E - Ef)/kT));
        fArr[idx] = f;
        EArr[idx] = E;
        rpUtilsProgress((int)((E-Emin)/(Emax-Emin)*100),"Iterating");
    }

    const char *curveLabel = "Fermi-Dirac Curve"
    const char *curveDesc = "Plot of Fermi-Dirac Calculation";

    // do it the easy way,
    // create a plot to add to the library
    // plot is registered with lib upon object creation
    // p1.add(nPts,xArr,yArr,format,curveLabel,curveDesc);

    Rappture::Plot p1(lib);
    p1.add(nPts,fArr,EArr,"",curveLabel,curveDesc);
    p1.propstr("xlabel","Fermi-Dirac Factor");
    p1.propstr("ylabel","Energy");
    p1.propstr("yunits","eV");

    lib.result();

    return 0;
}
