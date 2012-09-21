/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    [err] = rpLibResult(lib,status)
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
// METHOD: [err] = rpLibResult (libHandle,status)
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
    int status = 0;
    RpLibrary* lib = NULL;

    /* Check for proper number of arguments. */
    if (nrhs > 2) {
        mexErrMsgTxt("At most two input allowed.");
    }

    if (nrhs < 1) {
        mexErrMsgTxt("One input required.");
    }

    // grab the integer value of the library handle
    libIndex = getIntInput(prhs[0]);

    if (nrhs == 2) {
        status = getIntInput(prhs[1]);
    }

    /* Call the C subroutine. */
    if (libIndex > 0) {
        lib = (RpLibrary*) getObject_Void(libIndex);
        if (lib) {
            lib->put("tool.version.rappture.language", "matlab");
            lib->result(status);
            err = 0;
            // cleanLibDict();
        }
    }

    plhs[0] = mxCreateDoubleScalar(err);

    return;
}
