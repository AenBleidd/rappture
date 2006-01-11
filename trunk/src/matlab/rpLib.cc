/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    [libHandle,err] = rpLib(fileName)
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
// METHOD: [libHandle,err] = rpLib (fileName)
/// Opens a Rappture Library Object based on the xml file 'fileName'.
/**
 * Usually called in the beginning of a program to load an xml file
 * for future reading.
 */

void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{
    char *path;
    RpLibrary* lib = NULL;
    int libIndex = 0;
    int err = 1;

    /* Check for proper number of arguments. */
    if (nrhs != 1)
        mexErrMsgTxt("One input required.");
    else if (nlhs > 1)
        mexErrMsgTxt("Too many output arguments.");

    path = getStringInput(prhs[0]);

    /* Call the C subroutine. */
    if (path) {
        lib = rpLibrary(path);
        if (lib) {
            err = 0;
        }
    }

    libIndex = storeObject_Lib(lib);
    /* Set C-style string output_buf to MATLAB mexFunction output*/
    plhs[0] = mxCreateDoubleScalar(libIndex);
    plhs[1] = mxCreateDoubleScalar(err);

    return;
}
