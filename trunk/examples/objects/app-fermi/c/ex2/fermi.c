/*
 * ----------------------------------------------------------------------
 *  EXAMPLE: Fermi-Dirac function in C
 *
 *  This simple example shows how to use Rappture within a simulator
 *  written in C.
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include "rappture.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>

#include "fermi_io.c"

int main(int argc, char * argv[]) {

    /* declare variables to interact with Rappture */
    double T          = 0.0;
    double Ef         = 0.0;
    Rp_Plot *p1       = NULL;
    Rp_Plot *p2       = NULL;


    /* declare program variables */
    double E          = 0.0;
    double dE         = 0.0;
    double kT         = 0.0;
    double Emin       = 0.0;
    double Emax       = 0.0;
    double f          = 0.0;
    size_t nPts       = 200;
    size_t idx        = 0;
    double EArr[nPts];
    double fArr[nPts];

    /* initialize the global interface */
    Rp_InterfaceInit(argc,argv,&fermi_io);

    /* check the global interface for errors */
    if (Rp_InterfaceError() != 0) {
        /* there were errors while setting up the interface */
        /* dump the traceback */
        Rp_Outcome *o = Rp_InterfaceOutcome();
        fprintf(stderr, "%s", Rp_OutcomeContext(o));
        fprintf(stderr, "%s", Rp_OutcomeRemark(o));
        return(Rp_InterfaceError());
    }

    /*
     * connect variables to the interface
     * look in the global interface for an object named
     * "temperature", convert its value to Kelvin, and
     * store the value into the address of T.
     * look in the global interface for an object named
     * "Ef", convert its value to electron Volts and store
     * the value into the address of Ef.
     * look in the global interface for an object named
     * factorsTable and set the variable result to
     * point to it.
     */
    Rp_InterfaceConnect("temperature",&T,"units=K",NULL);
    Rp_InterfaceConnect("Ef",&Ef,"units=eV",NULL);
    Rp_InterfaceConnect("fdfPlot",p1,NULL);
    Rp_InterfaceConnect("fdfPlot2",p2,NULL);

    /* check the global interface for errors */
    if (Rp_InterfaceError() != 0) {
        /* there were errors while retrieving input data values */
        /* dump the traceback */
        Rp_Outcome *o = Rp_InterfaceOutcome();
        fprintf(stderr, "%s", Rp_OutcomeContext(o));
        fprintf(stderr, "%s", Rp_OutcomeRemark(o));
        return(Rp_InterfaceError());
    }

    /* do science calculations */
    kT = 8.61734e-5 * T;
    Emin = Ef - (10*kT);
    Emax = Ef + (10*kT);

    dE = (1.0/nPts)*(Emax-Emin);

    E = Emin;
    for(idx = 0; idx < nPts; idx++) {
        E = E + dE;
        f = 1.0/(1.0 + exp((E - Ef)/kT));
        fArr[idx] = f;
        EArr[idx] = E;
        Rp_UtilsProgress((int)((E-Emin)/(Emax-Emin)*100),"Iterating");
    }

    /*
     * set up the curves for the plot by using the add command
     * Rp_PlotAdd(plot,name,nPts,xdata,ydata,fmt);
     *
     * to group curves on the same plot, just keep adding curves
     * to save space, X array values are compared between curves.
     * the the X arrays contain the same values, we only store
     * one version in the internal data table, otherwise a new
     * column is created for the array. for big arrays this may take
     * some time, we should benchmark to see if this can be done
     * efficiently.
     */
    Rp_PlotAdd(p1,"fdfCurve1",nPts,fArr,EArr,"g:o");

    Rp_PlotAdd(p2,"fdfCurve2",nPts,fArr,EArr,"b-o");
    Rp_PlotAdd(p2,"fdfCurve3",nPts,fArr,EArr,"p--");

    /*
     * close the global interface
     * signal to the graphical user interface that science
     * calculations are complete and to display the data
     * as described in the views
     */
    Rp_InterfaceClose();

    return 0;
}
