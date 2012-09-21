/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    [unitsHandle,err] = rpUnitsFind(unitSymbol)
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
// METHOD: [unitHandle,err] = rpUnitsFind(unitSymbol)
/// Search the dictionary of Rappture Units for an instance named 'unitSymbol'
/**
 * This method searches the Rappture Units Dictionary for the
 * RpUnit named 'unitSymbol'. If it is found, its handle is`
 * returned, as a non-negative integer, to the caller for further use,
 * else a negative integer is returned signifying no unit of that`
 * name was found.
 * If unitSymbol is an empty string, unitHandle will be set to zero and err
 * will be set to a positive value.
 * Error code, err=0 on success, anything else is failure.
 */

void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{
    char *unitSymbol = NULL;
    const RpUnits* myUnit = NULL;
    int retHandle = 0;
    int err = 1;

    /* Check for proper number of arguments. */
    if (nrhs != 1)
        mexErrMsgTxt("Two input required.");
    else if (nlhs > 2)
        mexErrMsgTxt("Too many output arguments.");

    unitSymbol = getStringInput(prhs[0]);

    /* Call the C subroutine. */
    if (unitSymbol) {
        myUnit = RpUnits::find(unitSymbol);

        if (myUnit) {
            retHandle = storeObject_UnitsStr(myUnit->getUnitsName());
            if (retHandle) {
                err = 0;
            }
        }
    }

    /* Set C-style string output_buf to MATLAB mexFunction output*/
    plhs[0] = mxCreateDoubleScalar(retHandle);
    plhs[1] = mxCreateDoubleScalar(err);

    return;
}
