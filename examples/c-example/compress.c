// ======================================================================
//  Copyright (c) 2004-2007  Purdue Research Foundation
//  See the file "license.terms" for information on usage and
//  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
// ======================================================================

#include "rappture.h"

#include <stdio.h>

int main(int argc, char * argv[])
{

    RpLibrary* lib = NULL;

    const char* xmlFilePath = NULL;
    const char* dxFilePath = NULL;

    if (argc < 3) {
        printf("usage: %s driver.xml picture.dx\n", argv[0]);
    }

    xmlFilePath = argv[1];
    dxFilePath = argv[2];

    // create a rappture library from the file filePath
    lib = rpLibrary(xmlFilePath);

    if (lib == NULL) {
        // cannot open file or out of memory
        printf("FAILED creating Rappture Library\n");
        return 1;
    }

    // label the graph with a title
    rpPutString(lib,"output.field(dxFile1).about.label",
        "Example loading of a DX file",RPLIB_OVERWRITE);
    rpPutFile(lib,"output.field(dxFile1).component.dx",
        dxFilePath,RPLIB_BINARY,RPLIB_OVERWRITE);

    // write output to run file and signal
    rpResult(lib);

    return 0;
}
