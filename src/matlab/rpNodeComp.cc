/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    retStr = rpNodeComp(nodeHandle)
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
    const char *output_buf;
    int libIndex = 0;
    RpLibrary* lib = NULL;

    /* Check for proper number of arguments. */
    if (nrhs != 1)
        mexErrMsgTxt("One input required.");
    else if (nlhs > 1)
        mexErrMsgTxt("Too many output arguments.");

    // grab the integer value of the library handle
    libIndex = getIntInput(prhs[0]);

    /* Call the C subroutine. */
    if (libIndex > 0) {
        lib = getObject_Lib(libIndex);
        if (lib) {
            output_buf = rpNodeComp(lib);
        }
    }

    /* Set C-style string output_buf to MATLAB mexFunction output*/
    plhs[0] = mxCreateString(output_buf);

    return;
}
