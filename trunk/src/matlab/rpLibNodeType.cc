/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    [retStr,err] = rpLibNodeType(lib)
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
// METHOD: [retStr,err] = rpLibNodeType(nodeHandle)
/// Return the type name of the node represented by 'nodeHandle'
/**
 * This method returns the type name of the node represented`
 * by 'nodeHandle'.
 * Error code, err=0 on success, anything else is failure.
 */


void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{
    const char *output_buf;
    int libIndex = 0;
    int err = 1;
    RpLibrary* lib = NULL;

    /* Check for proper number of arguments. */
    if (nrhs != 1)
        mexErrMsgTxt("One input required.");
    else if (nlhs > 2)
        mexErrMsgTxt("Too many output arguments.");

    // grab the integer value of the library handle
    libIndex = getIntInput(prhs[0]);

    /* Call the C subroutine. */
    if (libIndex > 0) {
        lib = getObject_Lib(libIndex);
        if (lib) {
            output_buf = rpNodeType(lib);
            if (output_buf) {
                err = 0;
            }
        }
    }

    /* Set C-style string output_buf to MATLAB mexFunction output*/
    plhs[0] = mxCreateString(output_buf);
    plhs[1] = mxCreateDoubleScalar(err);

    return;
}
