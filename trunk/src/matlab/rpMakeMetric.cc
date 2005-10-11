/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    error = rpMakeMetric(basisHandle)
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
    RpUnits* basis = NULL;
    int retVal = 0;
    int basisHandle = 0;
    int retIndex = 0;

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
    }

    /* Set C-style string output_buf to MATLAB mexFunction output*/
    plhs[0] = mxCreateDoubleScalar(retVal);

    return;
}
