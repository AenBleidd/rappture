/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    [error] = rpUnitsMakeMetric(basisHandle)
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
// METHOD: [err] = rpUnitsMakeMetric(basisHandle)
/// Create metric extensions for the Rappture Unit referenced by basisHandle
/**
 * Create the metric extensions for the Rappture Unit, referenced`
 * by basisHandle, within Rappture's internal units dictionary.
 * Return an error code, err, to specify success or failure.
 * Error code, err=0 on success, anything else is failure.
 */

void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{
    const RpUnits* basis       = NULL;
    int            retVal      = 0;
    int            basisHandle = 0;
    int            retIndex    = 0;

    /* Check for proper number of arguments. */
    if (nrhs != 1)
        mexErrMsgTxt("Two input required.");
    else if (nlhs > 1)
        mexErrMsgTxt("Too many output arguments.");

    basisHandle = getIntInput(prhs[0]);

    /* Call the C subroutine. */
    if (basisHandle > 0) {
        basis = getObject_UnitsStr(basisHandle);

        if (basis) {
             retVal = RpUnits::makeMetric(basis);
        }
        else {
            std::string errMsg = "Error while retrieving basis handle.\n";
            errMsg += "Possibly invalid handle?\n";
            mexErrMsgTxt(errMsg.c_str());
        }
    }

    /* Set C-style string output_buf to MATLAB mexFunction output*/
    plhs[0] = mxCreateDoubleScalar(retVal);

    return;
}
