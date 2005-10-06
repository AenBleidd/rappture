#include "RpLibraryCInterface.h"

#include <stdlib.h>
#include <time.h>

int main(int argc, char * argv[])
{

    RpLibrary* lib = NULL;

    char* filePath;
    char* xmltext = NULL;
    double fmin, fmax;
    char strFormula[100];
    int i;

    double fx, fy;
    int npts = 100;
    char str[50];

    memset(strFormula, '\0', 100);

    if (argc < 2) {
        printf("usage: %s driver.xml\n", argv[0]);
    }

    filePath = argv[1];

    if (DEBUG)
        printf("filePath: %s:\n", filePath);

    // create a rappture library from the file filePath
    lib = library(argv[1]);

    if (lib) {
        if(DEBUG) {
            printf("created Rappture Library successfully\n");
        }
    }
    else {
        // cannot open file or out of memory
        printf("FAILED creating Rappture Library\n");
        exit(1);
    }

    // get the xml that is stored in the rappture library lib
    if( (xmltext = xml(lib)) ) {
        if(DEBUG) {
        //printf("XML file content:\n");
        //printf("%s\n", xmltext);
    }
    }
    else {
        printf("xml(lib) failed\n");
        exit(1);
    }

    // get the min
    xmltext = getString (lib, "input.number(min).current");

    if (! (xmltext) ) {
        printf("getString(lib,input.number(xmin).current) returns null\n");
        exit(1);
    }

    // if you want to keep the string around, you will need to malloc
    // space for it and strncpy() it to your own space. 
    // it will live in rappture's memory until the next call to getString()

    if(DEBUG) {
        printf("xml min: %s: len=%d\n", xmltext, strlen(xmltext));
    }

    fmin = atof(xmltext);
    // fmin = getDouble(lib,"input.number(min).current");
    if(DEBUG) {
        printf("min: %f\n", fmin);
    }

    // get the max
    fmax = getDouble(lib,"input.(max).current");
    if(DEBUG) {
        printf("max: %f\n", fmax);
    }

    // evaluate formula and generate results
    for (i = 0; i<npts; i++) {
        fx = i* (fmax - fmin)/npts + fmin;
        fy = sin(fx);
        sprintf(str, "%f %f\n", fx, fy);
        put(lib, "output.curve.component.xy", str, "result", 1);
    }


    // write output to run file and signal
    result(lib);

    return 0;
}
