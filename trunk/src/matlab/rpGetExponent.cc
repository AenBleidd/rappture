/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    retStr = rpGetExponent(unitsHandle)
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
    int         unitsHandle = 0;
    RpUnits*    unitsObj    = NULL;
    const char* retString   = NULL;
    double      retVal      = 0.0;

    /* Check for proper number of arguments. */
    if (nrhs != 1)
        mexErrMsgTxt("Two input required.");
    else if (nlhs > 1)
        mexErrMsgTxt("Too many output arguments.");

    unitsHandle = getIntInput(prhs[0]);

    /* Call the C subroutine. */
    if (unitsHandle > 0) {
        unitsObj = getObject_UnitsStr(unitsHandle);
        if (unitsObj) {
            retVal = unitsObj->getExponent();
        }
    }

    /* Set C-style string output_buf to MATLAB mexFunction output*/
    plhs[0] = mxCreateDoubleScalar(retVal);

    return;
}
