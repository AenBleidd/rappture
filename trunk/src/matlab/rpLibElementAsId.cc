/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    [retStr,err] = rpLibElementAsId(lib,path)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2005  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpMatlabInterface.h"

/**********************************************************************/
// METHOD: [retStr,err] = rpLibElementAsId(libHandle,path)
/// Return the id attribute of the element at location 'path' in 'libHandle'
/**
 * This method searches the Rappture Library Object 'libHandle' for the
 * node at the location described by the path 'path' and returns its
 * id attribute.
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
    char*       path = NULL;
    const char* retStr = NULL;

    /* Check for proper number of arguments. */
    if (nrhs != 2)
        mexErrMsgTxt("Two input required.");
    else if (nlhs > 2)
        mexErrMsgTxt("Too many output arguments.");

    libIndex = getIntInput(prhs[0]);
    path = getStringInput(prhs[1]);

    /* Call the C subroutine. */
    if ( (libIndex > 0) && (path) ) {
        lib = getObject_Lib(libIndex);
        if (lib) {
            retStr = rpElementAsId(lib,path);
            if (retStr) {
                err = 0;
            }
        }
    }

    /* Set C-style string output_buf to MATLAB mexFunction output*/
    plhs[0] = mxCreateString(retStr);
    plhs[1] = mxCreateDoubleScalar(err);

    return;
}
