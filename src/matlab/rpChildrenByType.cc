/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    nodeHandle = rpChildrenByType(libHandle,path,prevNodeHandle,type)
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

void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{
    int         libIndex = 0;
    int         childIndex = 0;
    int         retLibIndex = 0;
    RpLibrary*  lib = NULL;
    RpLibrary*  child = NULL;
    RpLibrary*  retLib = NULL;
    char*       path = NULL;
    char*       type = NULL;

    /* Check for proper number of arguments. */
    if (nrhs != 4)
        mexErrMsgTxt("Four input required.");
    else if (nlhs > 1)
        mexErrMsgTxt("Too many output arguments.");

    libIndex = getIntInput(prhs[0]);
    path = getStringInput(prhs[1]);
    childIndex = getIntInput(prhs[2]);
    type = getStringInput(prhs[3]);

    /* Call the C subroutine. */
    if ( (libIndex > 0) && (path) && (type) ) {
        lib = getObject_Lib(libIndex);

        if (childIndex > 0) {
            child = getObject_Lib(childIndex);
        }

        if (lib) {
            retLib = rpChildrenByType(lib,path,child,type);
            retLibIndex = storeObject_Lib(retLib);
        }
    }

    /* Set double scalar node handle to MATLAB mexFunction output*/
    plhs[0] = mxCreateDoubleScalar(retLibIndex);

    return;
}
