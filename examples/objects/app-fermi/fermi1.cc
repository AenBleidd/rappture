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

    Rappture::Library *lib = NULL;

    double T          = 0.0;
    double Ef         = 0.0;
    double E          = 0.0;
    double dE         = 0.0;
    double kT         = 0.0;
    double Emin       = 0.0;
    double Emax       = 0.0;
    double f          = 0.0;

    // create a rappture library from the file filePath
    lib = new Rappture::Library(argv[1]);

    if (lib == NULL) {
        // cannot open file or out of memory
        fprintf(stderr,"FAILED creating Rappture Library\n");
        return(1);
    }

    lib.value("temperature", &T, "units=K")
    lib.value("Ef", &Ef, "units=eV");

    if (lib.error() != 0) {
        // there were errors while retrieving input data values
        // dump the tracepack
        fprintf(stderr, lib.traceback());
        exit(lib.error());
    }

    kT = 8.61734e-5 * T;
    Emin = Ef - 10*kT;
    Emax = Ef + 10*kT;

    dE = 0.005*(Emax-Emin);

    Rappture::SimpleDoubleBuffer fBuf;
    Rappture::SimpleDoubleBuffer EBuf;

    for (E = Emin; E < Emax; E = E + dE) {
        f = 1.0/(1.0 + exp((E - Ef)/kT));
        fBuf.append(f);
        EBuf.append(E);
        rpUtilsProgress((int)((E-Emin)/(Emax-Emin)*100),"Iterating");
    }
    const double *fArr = fBuf.data();
    const double *EArr = EBuf.data();
    size_t nPts = fBuf.nmemb();

    const char *curveLabel = "Fermi-Dirac Curve"
    const char *curveDesc = "Plot of Fermi-Dirac Calculation";

    // do it the easy way,
    // create a plot and add to the library
    // p1->add(nPts,xArr,yArr,format,curveLabel,curveDesc);

    Rappture::Plot *p1 = new Rappture::Plot(lib);
    p1->add(nPts,fArr,EArr,"",curveLabel,curveDesc);
    p1.propstr("xlabel","Fermi-Dirac Factor");
    p1.propstr("ylabel","Energy");
    p1.propstr("yunits","eV");


    // or do it the harder way,
    // create a curve to add to the library
    // c1->axis("xaxis","xlabel","xdesc","units","scale",dataArr,nPts);

    Rappture::Curve *c1 = new Rappture::Curve(lib,"FDF");
    c1->label(curveLabel);
    c1->desc(curveDesc);
    c1->axis("xaxis","Fermi-Dirac Factor","","","",fArr,nPts);
    c1->axis("yaxis","Energy","","eV","",EArr,nPts);

    // add the curve to the library
    lib.put(c1);

    lib.result();

    // clean up memory
    delete lib;
    delete p1;
    delete c1;

    return 0;
}
