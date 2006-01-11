/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    [err] = rpLibResult(lib)
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
// METHOD: [err] = rpLibResult (libHandle)
/// Write Rappture Library to run.xml and signal end of processing.
/**
 * Usually the last call of the program, this function signals to the gui
 * that processing has completed and the output is ready to be
 * displayed
 */

void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{
    int libIndex = 0;
    int err = 1;
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
            rpResult(lib);
            err = 0;
            // cleanLibDict();
        }
    }

    plhs[0] = mxCreateDoubleScalar(err);

    return;
}
