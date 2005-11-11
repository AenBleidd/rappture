/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    [unitsHandle] = rpUnitsGetBasis(unitsHandle)
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
    RpUnits* myBasis = NULL;
    RpUnits* myUnit = NULL;
    int retHandle = 0;
    int unitsHandle = 0;

    /* Check for proper number of arguments. */
    if (nrhs != 1)
        mexErrMsgTxt("Two input required.");
    else if (nlhs > 1)
        mexErrMsgTxt("Too many output arguments.");

    unitsHandle = getIntInput(prhs[1]);

    /* Call the C subroutine. */
    if (unitsHandle > 0) {
        myUnit = getObject_UnitsStr(unitsHandle);

        if (myUnit) {

            myBasis = myUnit->getBasis();
            if (myBasis) {
                retHandle = storeObject_UnitsStr(myBasis->getUnitsName());
            }
        }
    }

    /* Set C-style string output_buf to MATLAB mexFunction output*/
    plhs[0] = mxCreateDoubleScalar(retHandle);
    return;
}
