/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Progress Source
 *
 *    [err] = rpUtilsProgress(percent,message)
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
// METHOD: [err] = rpUtilsProgress (percent,message)
/// Print the Rappture Progress message.
/**
 * Clients use this to generate the super secret Rappture
 * Progress message, and generate a progress bar in the
 * Rappture graphical user interface.
 */

void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{
    int         err = 1;
    int         percent = 0;
    const char* message = NULL;

    /* Check for proper number of arguments. */
    if (nrhs != 2) {
        mexErrMsgTxt("Two input required.");
    }

    percent = getIntInput(prhs[0]);
    message = getStringInput(prhs[1]);

    /* Call the C++ subroutine. */
    err = Rappture::Utils::progress(percent,message);

    plhs[0] = mxCreateDoubleScalar(err);

    freeStringInput((void*)message);
    rpmxFlush();
    return;
}
