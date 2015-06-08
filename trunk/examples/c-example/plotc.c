/*
 * ======================================================================
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include "rappture.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#ifdef DEBUG
static int debug = 1;
#else 
static int debug = 0;
#endif

int 
main(int argc, char **argv)
{

    RpLibrary* lib = NULL;

    const char* filePath;
    const char* xmltext = NULL;
    double fmin, fmax;
    char strFormula[100];
    int i;
    int err = 0;

    double fx, fy;
    int npts = 100;
    char str[50];

    memset(strFormula, '\0', 100);

    if (argc < 2) {
        printf("usage: %s driver.xml\n", argv[0]);
    }

    filePath = argv[1];

    if (debug)
        printf("filePath: %s:\n", filePath);

    // create a rappture library from the file filePath
    lib = rpLibrary(argv[1]);

    if (lib) {
        if(debug) {
            printf("created Rappture Library successfully\n");
        }
    }
    else {
        // cannot open file or out of memory
        printf("FAILED creating Rappture Library\n");
        return(1);
    }

    // get the xml that is stored in the rappture library lib
    err = rpXml(lib,&xmltext);
    if( !err ) {
        if(debug) {
        //printf("XML file content:\n");
        //printf("%s\n", xmltext);
        }
    }
    else {
        printf("xml(lib) failed\n");
        return(1);
    }

    // get the min
    rpGetString (lib, "input.number(min).current",&xmltext);

    if (! (xmltext) ) {
        printf("getString(lib,input.number(xmin).current) returns null\n");
        return(1);
    }

    // if you want to keep the string around, you will need to malloc
    // space for it and strncpy() it to your own space. 
    // it will live in rappture's memory until the next call to getString()

    if(debug) {
      printf("xml min: %s: len=%d\n", xmltext, (int)strlen(xmltext));
    }

    fmin = atof(xmltext);
    // fmin = getDouble(lib,"input.number(min).current");
    if(debug) {
        printf("min: %f\n", fmin);
    }

    // get the max
    rpGetDouble(lib,"input.(max).current",&fmax);
    if(debug) {
        printf("max: %f\n", fmax);
    }

    // label the graph with a title
    rpPutString(lib,"output.curve(result).about.label",
        "Formula: Y Vs X",RPLIB_OVERWRITE);

    // evaluate formula and generate results
    for (i = 0; i<npts; i++) {
        fx = i* (fmax - fmin)/npts + fmin;
        fy = sin(fx);
        sprintf(str, "%f %f\n", fx, fy);
        rpPutString(lib,"output.curve(result).component.xy",str,RPLIB_APPEND);
    }


    // write output to run file and signal
    rpResult(lib);

    // free the rappture library
    rpFreeLibrary(&lib);

    // exit program
    return 0;
}
