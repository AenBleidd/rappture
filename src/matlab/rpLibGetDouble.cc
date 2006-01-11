/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    [nodeHandle,err] = rpLibGetDouble(libHandle,path)
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
// METHOD: [retStr,err] = rpLibGetDouble(libHandle,path)
/// Query the value of a node.
/**
 * Clients use this to query the value of a node.  If the path
 * is not specified, it returns the value associated with the
 * root node.  Otherwise, it returns the value for the element
 * specified by the path. The return value is returned as a double.
 *
 * Error code, err=0 on success, anything else is failure.
 */

void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{
    int         libIndex    = 0;
    int         retLibIndex = 0;
    int         err         = 1;
    RpLibrary*  lib         = NULL;
    char*       path        = NULL;
    double      retVal      = 0.0;

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
            retVal = rpGetDouble(lib,path);
            err = 0;
        }
    }

    /* Set double scalar to MATLAB mexFunction output*/
    plhs[0] = mxCreateDoubleScalar(retVal);
    plhs[1] = mxCreateDoubleScalar(err);

    return;
}
