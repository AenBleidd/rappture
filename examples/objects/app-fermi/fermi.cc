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

    int err           = 0;

    // create a rappture library from the file filePath
    lib = new Rappture::Library(argv[1]);

    if (lib == NULL) {
        // cannot open file or out of memory
        fprintf(stderr,"FAILED creating Rappture Library\n");
        return(1);
    }


    /* Alternative ways to access data

    //////////////////////////////
    lib.get("input.number(temperature)").value("K",&T)
    lib.get("input.number(Ef)").value("eV",&Ef);
    //////////////////////////////

    //////////////////////////////
    Rappture::Number *rpT = NULL;
    Rappture::Number *rpEf = NULL;
    rpT = (Rappture::Number *) lib.get("input.number(temperature)");
    rpEf = (Rappture::Number *) lib.get("input.number(Ef)");

    T = rpT->value("K");
    Ef = rpEf->value("Ef");
    //////////////////////////////

    //////////////////////////////
    T = lib.value("input.number(temperature)","units=K")
    Ef = lib.value("input.number(Ef)","units=eV");
    //////////////////////////////
    */

    int err = 0;
    err = lib.value("input.number(temperature)", &T, "units=K")
    err = lib.value("input.number(Ef)", &Ef, "units=eV");

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
    // create a plot to add to the library
    // p1->add(nPts,xArr,yArr,format,curveLabel,curveDesc);

    Rappture::Plot *p1 = new Rappture::Plot();
    p1->add(nPts,fArr,EArr,"",curveLabel,curveDesc);
    p1.propstr("xlabel","Fermi-Dirac Factor");
    p1.propstr("ylabel","Energy");
    p1.propstr("yunits","eV");

    // add the plot to the library
    lib.put(p1);


    // or do it the harder way,
    // create a curve to add to the library
    // c1->axis("xaxis","xlabel","xdesc","units","scale",dataArr,nPts);

    Rappture::Curve *c1 = new Rappture::Curve("output.curve(FDF)");
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
