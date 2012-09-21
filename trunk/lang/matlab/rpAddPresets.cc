/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    result = rpAddPresets(presetName)
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

void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{
    std::string presetName  = "";
    int result = -1;

    /* Check for proper number of arguments. */
    if (nrhs != 1)
        mexErrMsgTxt("Two input required.");
    else if (nlhs > 1)
        mexErrMsgTxt("Too many output arguments.");

    presetName = getStringInput(prhs[0]);

    /* Call the C subroutine. */
    if ( !presetName.empty() ) {
        result = RpUnits::addPresets(presetName);
    }

    /* Set output to MATLAB mexFunction output */
    plhs[0] = mxCreateDoubleScalar((double)result);

    return;
}
