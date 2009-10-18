
// ----------------------------------------------------------------------
//  EXAMPLE: Fermi-Dirac function in Python.
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

    Rappture_Library *lib = NULL;
    Rappture_Number *T = NULL;

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

    const char *curveLabel = "Fermi-Dirac Curve"
    const char *curveDesc = "Plot of Fermi-Dirac Calculation";

    Rappture::Plot *p1 = NULL;

    // create a rappture library from the file filePath
    lib = Rappture_Library_init(argv[1]);

    if (Rappture_Library_error(lib) != 0) {
        // cannot open file or out of memory
        fprintf(stderr,Rappture_Library_traceback(lib));
        return(Rappture_Library_error(lib));
    }

    Rappture_connect(lib,"temperature",T);
    Rappture_Library_value(lib,"Ef",&Ef,"units=eV");

    if (Rappture_Library_error(lib) != 0) {
        // there were errors while retrieving input data values
        // dump the tracepack
        fprintf(stderr,Rappture_Library_traceback(lib));
        return(Rappture_Library_error(lib));
    }

    kT = 8.61734e-5 * Rappture_Number_value(T,"K");
    Emin = Ef - 10*kT;
    Emax = Ef + 10*kT;

    dE = 0.005*(Emax-Emin);

    E = Emin;
    for(size_t idx = 0; idx < nPts; idx++) {
        E = E + dE;
        f = 1.0/(1.0 + exp((E - Ef)/kT));
        fArr[idx] = f;
        EArr[idx] = E;
        Rappture_Utils_Progress((int)((E-Emin)/(Emax-Emin)*100),"Iterating");
    }

    // do it the easy way,
    // create a plot to add to the library
    // plot is registered with lib upon object creation
    // p1->add(nPts,xArr,yArr,format,curveLabel,curveDesc);

    p1 = Rappture_Plot_init(lib);
    Rappture_Plot_add(p1,nPts,fArr,EArr,"",curveLabel,curveDesc);
    Rappture_Plot_propstr(p1,"xlabel","Fermi-Dirac Factor");
    Rappture_Plot_propstr(p1,"ylabel","Energy");
    Rappture_Plot_propstr(p1,"yunits","eV");

    Rappture_Library_result(lib);
    return 0;
}
