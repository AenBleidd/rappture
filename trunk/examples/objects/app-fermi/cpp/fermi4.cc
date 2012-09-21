// ----------------------------------------------------------------------
//  EXAMPLE: Fermi-Dirac function in C++.
//
//  This simple example shows how to use Rappture within a simulator
//  written in C++.
// ======================================================================
//  AUTHOR:  Derrick Kearney, Purdue University
//  Copyright (c) 2004-2012  HUBzero Foundation, LLC
//
//  See the file "license.terms" for information on usage and
//  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
// ======================================================================

#include "rappture.h"

#include <cstdlib>
#include <cstdio>
#include <math.h>
#include <unistd.h>

#include "fermi_io.c"

int main(int argc, char * argv[]) {

    // declare variables to interact with Rappture
    double T            = 0.0;
    double Ef           = 0.0;
    Rappture::Table *result = NULL;

    // declare program variables
    double E            = 0.0;
    double dE           = 0.0;
    double kT           = 0.0;
    double Emin         = 0.0;
    double Emax         = 0.0;
    double f            = 0.0;
    size_t nPts         = 200;
    double EArr[nPts];
    double fArr[nPts];

    // initialize the global interface
    Rappture::Interface(argc,argv,&fermi_io);

    //check the global interface for errors
    if (Rappture::Interface::error() != 0) {
        // there were errors while setting up the interface
        // dump the traceback
        Rappture::Outcome o = Rappture::Interface::outcome();
        fprintf(stderr, "%s",o.context());
        fprintf(stderr, "%s",o.remark());
        return (Rappture::Interface::error());
    }

    // connect variables to the interface
    // look in the global interface for an object named
    // "temperature", convert its value to Kelvin, and
    // store the value into the address of T.
    // look in the global interface for an object named
    // "Ef", convert its value to electron Volts and store
    // the value into the address of Ef
    // look in the global interface for an object named
    // factorsTable and set the variable result to
    // point to it.
    Rappture::Interface::connect("temperature",&T,"units=K",NULL);
    Rappture::Interface::connect("Ef",&Ef,"units=eV",NULL);
    Rappture::Interface::connect("factorsTable",result,NULL);

    // check the global interface for errors
    if (Rappture::Interface::error() != 0) {
        // there were errors while retrieving input data values
        // dump the tracepack
        Rappture::Outcome o = Rappture::Interface::outcome();
        fprintf(stderr, "%s",o.context());
        fprintf(stderr, "%s",o.remark());
        return(Rappture::Interface::error());
    }

    // do science calculations
    kT = 8.61734e-5 * T;
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

    // store results in the results table
    // add data to the table pointed to by the variable result.
    // put the fArr data in the column named "Fermi-Dirac Factor"
    // put the EArr data in the column named "Energy"

    result->addData("Fermi-Dirac Factor",nPts,fArr);
    result->addData("Energy",nPts,EArr);

    // close the global interface
    // signal to the graphical user interface that science
    // calculations are complete and to display the data
    // as described in the views
    Rappture::Interface::close();

    return 0;
}
