// ----------------------------------------------------------------------
//  EXAMPLE: Fermi-Dirac function in Python.
//
//  This simple example shows how to use Rappture within a simulator
//  written in Python.
// ======================================================================
//  AUTHOR:  Michael McLennan, Purdue University
//  Copyright (c) 2004-2005  Purdue Research Foundation
//
//  See the file "license.terms" for information on usage and
//  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
// ======================================================================

#include "RpLibraryCInterface.h"
#include "RpUnitsCInterface.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

int main(int argc, char * argv[]) {

    RpLibrary* lib    = NULL;

    const char* data  = NULL;
    char line[100];

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
    lib = rpLibrary(argv[1]);

    if (lib == NULL) {
        // cannot open file or out of memory
        printf("FAILED creating Rappture Library\n");
        return(1);
    }


    rpGetString(lib,"input.(temperature).current",&data);
    T = rpConvertDbl(data, "K", &err);
    if (err) {
        printf ("Error while retrieving input.(temperature).current\n");
        return(1);
    }


    rpGetString(lib,"input.(Ef).current",&data);
    Ef = rpConvertDbl(data, "eV", &err);
    if (err) {
        printf ("Error while retrieving input.(Ef).current\n");
        return(1);
    }

    kT = 8.61734e-5 * T;
    Emin = Ef - 10*kT;
    Emax = Ef + 10*kT;

    E = Emin;
    dE = 0.005*(Emax-Emin);

    while (E < Emax) {
        f = 1.0/(1.0 + exp((E - Ef)/kT));
        sprintf(line,"%f %f\n",f, E);
        rpPutString(lib,"output.curve(f12).component.xy", line, 1);
        E = E + dE;
    }

    rpResult(lib);
    return 0;
}
