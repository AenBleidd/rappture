/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    [nodeHandle,err] = rpLibElementAsObject(libHandle,path)
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
// METHOD: [retStr,err] = rpLibElementAsObject(libHandle,path)
/// Return a handle to the element at location 'path' in 'libHandle'
/**
 * This method searches the Rappture Library Object 'libHandle' for the
 * node at the location described by the path 'path' and returns
 * a handle to it.
 *
 * If path is an empty string, the root of the node is used. 'libHandle'
 * is the handle representing the instance of the RpLibrary object.
 * Error code, err=0 on success, anything else is failure.
 */

void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{
    int         libIndex = 0;
    int         retLibIndex = 0;
    int         err = 1;
    RpLibrary*  lib = NULL;
    RpLibrary*  retLib = NULL;
    std::string path = "";

    /* Check for proper number of arguments. */
    if (nrhs != 2) {
        mexErrMsgTxt("Two input required.");
    }

    libIndex = getIntInput(prhs[0]);
    path = getStringInput(prhs[1]);

    /* Call the C++ subroutine. */
    if ( (libIndex > 0) && (!path.empty()) ) {
        lib = (RpLibrary*) getObject_Void(libIndex);
        if (lib) {
            retLib = lib->element(path);
            retLibIndex = storeObject_Void((void*)retLib);
            if (retLibIndex) {
                err = 0;
            }
        }
    }

    /* Set double scalar node handle to MATLAB mexFunction output*/
    plhs[0] = mxCreateDoubleScalar(retLibIndex);
    plhs[1] = mxCreateDoubleScalar(err);

    return;
}
