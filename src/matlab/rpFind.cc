/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    unitsHandle = rpFind(unitSymbol)
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
    char *unitSymbol = NULL;
    RpUnits* myUnit = NULL;
    int retHandle = 0;

    /* Check for proper number of arguments. */
    if (nrhs != 1)
        mexErrMsgTxt("Two input required.");
    else if (nlhs > 1)
        mexErrMsgTxt("Too many output arguments.");

    unitSymbol = getStringInput(prhs[0]);

    /* Call the C subroutine. */
    if (unitSymbol) {
        myUnit = RpUnits::find(unitSymbol);

        if (myUnit) {
            retHandle = storeObject_UnitsStr(myUnit->getUnitsName());
        }
    }

    /* Set C-style string output_buf to MATLAB mexFunction output*/
    plhs[0] = mxCreateDoubleScalar(retHandle);

    return;
}
