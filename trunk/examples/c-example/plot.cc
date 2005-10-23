// ======================================================================
//  Copyright (c) 2004-2005  Purdue Research Foundation
//  See the file "license.terms" for information on usage and
//  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
// ======================================================================

#include "RpLibrary.h"

#include <stdlib.h>
#include <math.h>

#include <iostream>
#include <sstream>

int main(int argc, char * argv[])
{

    RpLibrary* lib = NULL;

    std::string filePath = "";
    std::string xmltext = "";
    double fmin, fmax;
    std::string strFormula = "";
    int i;

    if (argc < 2) {
        std::cout << "usage: " << argv[0] << " driver.xml" << std::endl;
    }

    filePath = std::string(argv[1]);

    if (DEBUG) {
        std::cout << "filePath: " << filePath << std::endl;
    }

    // create a rappture library from the file filePath
    lib = new RpLibrary(filePath);

    if (lib) {
        if(DEBUG) {
            std::cout << "created Rappture Library successfully" << std::endl;
        }
    }
    else {
        // cannot open file or out of memory
        std::cout << "FAILED creating Rappture Library" << std::endl;
        exit(1);
    }

    // get the xml that is stored in the rappture library lib
    xmltext = lib->xml();

    if(! (xmltext.empty()) ) {
        if(DEBUG) {
        //printf("XML file content:\n");
        //printf("%s\n", xmltext);
    }
    }
    else {
        printf("lib->xml() failed\n");
        exit(1);
    }

    // get the min
    xmltext = lib->getString("input.number(min).current");

    if ( xmltext.empty() ) {
        std::cout << "lib->getString(input.number(xmin).current) returns null" << std::endl;
        exit(1);
    }

    if(DEBUG) {
        std::cout << "xml min :" << xmltext << ": len=" << xmltext.size() << std::endl;
    }

    // grab a double value from the xml
    fmin = lib->getDouble("input.number(min).current");

    if(DEBUG) {
        std::cout << "min: " << fmin << std::endl;
    }

    // get the max
    fmax = lib->getDouble("input.(max).current");
    if(DEBUG) {
        std::cout << "max: " << fmax << std::endl;
    }

    // evaluate formula and generate results
    // science begains here
    double fx, fy;
    int npts = 100;
    std::stringstream myStr;

    for (i = 0; i<npts; i++) {
        fx = i* (fmax - fmin)/npts + fmin;
        fy = sin(fx);
        myStr << fx << " " << fy << std::endl;
        lib->put("output.curve.component.xy", myStr.str(), "result", 1);
    }


    // write output to run file and signal
    lib->result();

    return 0;
}
