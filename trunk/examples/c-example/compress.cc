// ======================================================================
//  Copyright (c) 2004-2012  HUBzero Foundation, LLC
//  See the file "license.terms" for information on usage and
//  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
// ======================================================================

#include "rappture.h"

#include <stdlib.h>
#include <iostream>

int main(int argc, char * argv[])
{

    RpLibrary* lib = NULL;

    std::string xmlFilePath = "";
    std::string dxFilePath = "";

    if (argc < 3) {
        std::cout << "usage: " << argv[0] << " driver.xml picture.dx" << std::endl;
    }

    xmlFilePath = std::string(argv[1]);
    dxFilePath = std::string(argv[2]);

    // create a rappture library from the file filePath
    lib = new RpLibrary(xmlFilePath);

    if (lib == NULL) {
        // cannot open file or out of memory
        std::cout << "FAILED creating Rappture Library" << std::endl;
        exit(1);
    }

    // label the graph with a title
    lib->put("output.field(dxFile1).about.label",
        "Example loading of a DX file","",RPLIB_OVERWRITE);
    lib->putFile("output.field(dxFile1).component.dx",
        dxFilePath,RPLIB_COMPRESS,RPLIB_OVERWRITE);


    // write output to run file and signal
    lib->result();
    delete lib;

    return 0;
}
