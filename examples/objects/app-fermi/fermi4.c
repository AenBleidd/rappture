// ----------------------------------------------------------------------
//  EXAMPLE: Fermi-Dirac function in C
//
//  This simple example shows how to use Rappture within a simulator
//  written in C.
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

    Rp_Library *lib = NULL;
    Rp_Number *T = NULL;

    double Ef         = 0.0;
    double E          = 0.0;
    double dE         = 0.0;
    double kT         = 0.0;
    double Emin       = 0.0;
    double Emax       = 0.0;
    double f          = 0.0;
    size_t nPts       = 200;
    double EArr[nPts];
    double fArr[nPts];

    Rp_Plot *p1 = NULL;

    // create a rappture library from the file filePath
    lib = Rp_LibraryInit();
    Rp_LibraryLoadFile(lib,argv[1]);

    if (Rp_LibraryError(lib) != 0) {
        // cannot open file or out of memory
        Rp_Outcome *o = Rp_LibraryOutcome(lib);
        fprintf(stderr, "%s", Rp_OutcomeContext(o));
        fprintf(stderr, "%s", Rp_OutcomeRemark(o));
        return(Rp_LibraryError(lib));
    }

    Rp_Connect(lib,"temperature",T);
    Rp_LibraryValue(lib,"Ef",&Ef,1,"units=eV");

    if (Rp_LibraryError(lib) != 0) {
        // there were errors while retrieving input data values
        // dump the tracepack
        Rp_Outcome *o = Rp_LibraryOutcome(lib);
        fprintf(stderr, "%s", Rp_OutcomeContext(o));
        fprintf(stderr, "%s", Rp_OutcomeRemark(o));
        return(Rp_LibraryError(lib));
    }

    kT = 8.61734e-5 * Rp_NumberValue(T,"K");
    Emin = Ef - 10*kT;
    Emax = Ef + 10*kT;

    dE = (1.0/nPts)*(Emax-Emin);

    E = Emin;
    for(size_t idx = 0; idx < nPts; idx++) {
        E = E + dE;
        f = 1.0/(1.0 + exp((E - Ef)/kT));
        fArr[idx] = f;
        EArr[idx] = E;
        Rp_UtilsProgress((int)((E-Emin)/(Emax-Emin)*100),"Iterating");
    }

    // do it the easy way,
    // create a plot to add to the library
    // plot is registered with lib upon object creation
    // p1->add(nPts,xArr,yArr,format,name);

    p1 = Rp_PlotInit(lib);
    Rp_PlotAdd(p1,nPts,fArr,EArr,"","fdfactor");
    Rp_PlotPropstr(p1,"label","Fermi-Dirac Curve");
    Rp_PlotPropstr(p1,"desc","Plot of Fermi-Dirac Calculation");
    Rp_PlotPropstr(p1,"xlabel","Fermi-Dirac Factor");
    Rp_PlotPropstr(p1,"ylabel","Energy");
    Rp_PlotPropstr(p1,"yunits","eV");

    Rp_LibraryResult(lib);

    return 0;
}
