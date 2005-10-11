/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    nodeHandle = rpChildren(libHandle,path,prevNodeHandle)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
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

    /* Check for proper number of arguments. */
    if (nrhs != 3)
        mexErrMsgTxt("Two input required.");
    else if (nlhs > 1)
        mexErrMsgTxt("Too many output arguments.");

    libIndex = getIntInput(prhs[0]);
    path = getStringInput(prhs[1]);
    childIndex = getIntInput(prhs[2]);

    /* Call the C subroutine. */
    if ( (libIndex > 0) && (path) ) {
        lib = getObject_Lib(libIndex);

        if (childIndex > 0) {
            child = getObject_Lib(childIndex);
        }

        if (lib) {
            retLib = rpChildren(lib,path,child);
            retLibIndex = storeObject_Lib(retLib);
        }
    }

    /* Set double scalar node handle to MATLAB mexFunction output*/
    plhs[0] = mxCreateDoubleScalar(retLibIndex);

    return;
}
