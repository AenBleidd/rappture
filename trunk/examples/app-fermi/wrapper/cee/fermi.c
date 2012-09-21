/*
 *----------------------------------------------------------------------
 * EXAMPLE: Fermi-Dirac function in C.
 *
 * This simple example shows how to use Rappture to wrap up a
 * simulator written in Matlab or some other language.
 *
 *======================================================================
 * AUTHOR:  Derrick Kearney, Purdue University
 * Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *======================================================================
 */

#include "rappture.h"
#include <stdlib.h>
#include <stdio.h>

enum prog_consts { MAXLEN = 1000 };

int main(int argc, char * argv[]) {

    RpLibrary *lib    = NULL;

    double T          = 0.0;
    double Ef         = 0.0;

    int err           = 0;
    int skipCnt       = 4;

    FILE *fp          = NULL;

    char *prog        = argv[0]; /* program name for errors */
    const char *data  = NULL;
    char line[MAXLEN+1];

    // create a rappture library from the file filePath
    lib = rpLibrary(argv[1]);

    if (lib == NULL) {
        // cannot open file or out of memory
        fprintf(stderr,"%s: FAILED creating Rappture Library\n",prog);
        exit(1);
    }

    rpGetString(lib,"input.(temperature).current",&data);
    T = rpConvertDbl(data, "K", &err);
    if (err) {
        fprintf(stderr, "%s: Error retrieving current temperature\n",prog);
        exit(err);
    }

    rpGetString(lib,"input.(Ef).current",&data);
    Ef = rpConvertDbl(data, "eV", &err);
    if (err) {
        fprintf(stderr,"%s: Error while retrieving current EF\n",prog);
        exit(err);
    }

    fp = fopen("indeck","w");
    if (fp == NULL) {
        fprintf(stderr,"%s: Error while writing to indeck",prog);
        exit(1);
    }

    fprintf(fp,"%f\n%f",Ef,T);
    fclose(fp);
    fp = NULL;

    // run the science program and capture the exit status
    err = system("octave --silent fermi.m < indeck");

    if (err == 1) {
        fprintf(stderr,"%s: calling octave science program failed",prog);
        exit(err);
    }

    // Label output graph with title, x-axis label,
    // y-axis label, and y-axis units
    rpPutString (   lib,
                    "output.curve(f12).about.label",
                    "Fermi-Dirac Factor",
                    RPLIB_OVERWRITE );
    rpPutString (   lib,
                    "output.curve(f12).xaxis.label",
                    "Fermi-Dirac Factor",
                    RPLIB_OVERWRITE );
    rpPutString (   lib,
                    "output.curve(f12).yaxis.label",
                    "Energy",
                    RPLIB_OVERWRITE );
    rpPutString(lib, "output.curve(f12).yaxis.units", "eV", RPLIB_OVERWRITE);

    fp = fopen("out.dat","r");
    if (fp == NULL) {
        fprintf(stderr,"%s: Error while writing to out.dat",prog);
        exit(1);
    }

    while (skipCnt > 0) {
        fgets(line,MAXLEN,fp);
        skipCnt--;
    }

    // it turns out that the octave script places the x and y values
    // in the correct format needed by rappture which is the x value
    // followed by a space followed by the y value and ended with a 
    // newline character. this allows us to just read each line and
    // place it into our rappture object at the correct location
    // output.curve(f12).component.xy
    while (fgets(line,MAXLEN,fp)) {
       rpPutString(lib,"output.curve(f12).component.xy",line,RPLIB_APPEND);
    }

    remove("indeck");
    remove("out.dat");

    rpResult(lib);
    exit (0);
}
