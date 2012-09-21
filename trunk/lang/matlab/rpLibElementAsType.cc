/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    [retStr,err] = rpLibElementAsType(lib,path)
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
// METHOD: [retStr,err] = rpLibElementAsType(libHandle,path)
/// Return the type name of the element at location 'path' in 'libHandle'
/**
 * This method searches the Rappture Library Object 'libHandle' for the
 * node at the location described by the path 'path' and returns its
 * type name.
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
    RpLibrary*  eleLib = NULL;
    std::string path = "";
    std::string retStr = "";

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
            eleLib = lib->element(path);
            if (eleLib) {
                retStr = eleLib->nodeType();
                if (!retStr.empty()) {
                    err = 0;
                }
            }
        }
    }

    /* Set C-style string output_buf to MATLAB mexFunction output*/
    plhs[0] = mxCreateString(retStr.c_str());
    plhs[1] = mxCreateDoubleScalar(err);

    return;
}
