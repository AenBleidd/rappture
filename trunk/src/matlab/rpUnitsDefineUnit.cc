/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    [unitsHandle] = rpUnitsDefineUnit(unitSymbol, basisHandle)
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
    const RpUnits* basis = NULL;
    RpUnits* myUnit = NULL;
    int retHandle = 0;
    int basisHandle = 0;
    int retIndex = 0;

    /* Check for proper number of arguments. */
    if (nrhs != 2)
        mexErrMsgTxt("Two input required.");
    else if (nlhs > 1)
        mexErrMsgTxt("Too many output arguments.");

    unitSymbol = getStringInput(prhs[0]);
    basisHandle = getIntInput(prhs[1]);

    /* Call the C subroutine. */
    if (basisHandle > 0) {
        basis = getObject_UnitsStr(basisHandle);
    }

    if (unitSymbol) {
        myUnit = RpUnits::define(unitSymbol,basis);

        if (myUnit) {
            retHandle = storeObject_UnitsStr(myUnit->getUnitsName());
        }
    }

    /* Set C-style string output_buf to MATLAB mexFunction output*/
    plhs[0] = mxCreateDoubleScalar(retHandle);

    return;
}
