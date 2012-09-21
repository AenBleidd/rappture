/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    [unitsHandle,err] = rpUnitsGetBasis(unitsHandle)
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
// METHOD: [basisHandle,err] = rpUnitsGetBasis(unitHandle)
/// Return a handle to the basis of the provided instance of a Rappture Unit.
/**
 * Retrieve the basis of the Rappture Units object with the handle`
 * 'unitHandle'. Return the handle of the basis if it exists. If there`
 * is no basis, then return a negative integer.
 * Error code, err=0 on success, anything else is failure.
 */

void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{
    char *unitSymbol = NULL;
    const RpUnits* myBasis = NULL;
    const RpUnits* myUnit = NULL;
    int retHandle = 0;
    int unitsHandle = 0;
    int err = 1;

    /* Check for proper number of arguments. */
    if (nrhs != 1)
        mexErrMsgTxt("Two input required.");
    else if (nlhs > 2)
        mexErrMsgTxt("Too many output arguments.");

    unitsHandle = getIntInput(prhs[0]);

    /* Call the C subroutine. */
    if (unitsHandle > 0) {
        myUnit = getObject_UnitsStr(unitsHandle);

        if (myUnit) {

            myBasis = myUnit->getBasis();
            if (myBasis) {
                // there was a basis, store it an return
                retHandle = storeObject_UnitsStr(myBasis->getUnitsName());
                if (retHandle) {
                    err = 0;
                }
            }
            else {
                // there was no basis for this unit
                // return negative handle and set err = 0
                retHandle = -1;
                err = 0;
            }
        }
    }

    /* Set C-style string output_buf to MATLAB mexFunction output*/
    plhs[0] = mxCreateDoubleScalar(retHandle);
    plhs[1] = mxCreateDoubleScalar(err);
    return;
}
