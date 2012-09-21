/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    [err] = rpLibPutData (libHandle,path,bytes,nbytes,append)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpMatlabInterface.h"

/**********************************************************************/
// METHOD: [err] = rpLibPutData (libHandle,path,bytes,nbytes,append)
/// Set the value of a node.
/**
 * Clients use this to set the value of a node.  If the path
 * is not specified, it sets the value for the root node.
 * Otherwise, it sets the value for the element specified
 * by the path.  The value is treated as the text within the
 * tag at the tail of the path.
 *
 * FileName is the name of the file to import into the rappture object
 * Compress is an integer telling if you want the data compressed (use 1)
 * or uncompressed (use 0).
 * Append is an integer telling if this new data should overwrite
 * (use 0) or be appended (use 1) to previous data in this node.
 *
 */

void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{
    int         libIndex = 0;
    int         append = 0;
    int         nbytes = 0;
    int         err = 1;
    RpLibrary*  lib = NULL;
    std::string path = "";
    std::string bytes = "";

    /* Check for proper number of arguments. */
    if (nrhs != 5) {
        mexErrMsgTxt("Five inputs required.");
    }

    libIndex = getIntInput(prhs[0]);
    path = getStringInput(prhs[1]);
    bytes = getStringInput(prhs[2]);
    nbytes = getIntInput(prhs[3]);
    append = getIntInput(prhs[4]);

    /* Call the C++ subroutine. */
    if ( (libIndex > 0) && (!path.empty()) ) {
        lib = (RpLibrary*) getObject_Void(libIndex);

        if (lib) {
            lib->putData(path,bytes.c_str(),nbytes,append);
            err = 0;
        }
    }

    plhs[0] = mxCreateDoubleScalar(err);

    return;
}
