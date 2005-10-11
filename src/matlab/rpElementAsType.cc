/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    retStr = rpElementAsType(lib,path)
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
    int         retLibIndex = 0;
    RpLibrary*  lib = NULL;
    char*       path = NULL;
    const char* retStr = NULL;

    /* Check for proper number of arguments. */
    if (nrhs != 2)
        mexErrMsgTxt("Two input required.");
    else if (nlhs > 1)
        mexErrMsgTxt("Too many output arguments.");

    libIndex = getIntInput(prhs[0]);
    path = getStringInput(prhs[1]);

    /* Call the C subroutine. */
    if ( (libIndex > 0) && (path) ) {
        lib = getObject_Lib(libIndex);
        if (lib) {
            retStr = rpElementAsType(lib,path);
        }
    }

    /* Set C-style string output_buf to MATLAB mexFunction output*/
    plhs[0] = mxCreateString(retStr);

    return;
}
