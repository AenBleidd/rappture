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

#include <cstdlib>
#include <cstdio>
#include <math.h>
#include <unistd.h>

int main(int argc, char * argv[]) {

    // create a rappture library from the file filePath
    Rappture::Library lib;
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

    lib.loadFile(argv[1]);
    if (lib.error() != 0) {
        // cannot open file or out of memory
        Rappture::Outcome o = lib.outcome();
        fprintf(stderr, "%s",o.context());
        fprintf(stderr, "%s",o.remark());
        return (lib.error());
    }

    Rappture::Connect(&lib,"temperature",T);
    lib.value("Ef", &Ef, 1, "units=eV");

    if (lib.error() != 0) {
        // there were errors while retrieving input data values
        // dump the tracepack
        Rappture::Outcome o = lib.outcome();
        fprintf(stderr, "%s",o.context());
        fprintf(stderr, "%s",o.remark());
        return(lib.error());
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
        Rappture::Utils::progress((int)((E-Emin)/(Emax-Emin)*100),"Iterating");
    }

    // do it the easy way,
    // create a plot to add to the library
    // plot is registered with lib upon object creation
    // p1.add(nPts,xArr,yArr,format,name);

    Rappture::Plot p1(lib);
    p1.add(nPts,fArr,EArr,"","fdfactor");
    p1.propstr("label","Fermi-Dirac Curve");
    p1.propstr("desc","Plot of Fermi-Dirac Calculation");
    p1.propstr("xlabel","Fermi-Dirac Factor");
    p1.propstr("ylabel","Energy");
    p1.propstr("yunits","eV");

    lib.result();

    return 0;
}
