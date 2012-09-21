/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    [retStr,err] = rpUnitsGetUnitsName(unitsHandle)
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
// METHOD: [retStr,err] = rpUnitsGetUnitsName(unitHandle)
/// Return the unit and exponent of the Rappture Unit represented by unitHandle.
/**
 * Retrieve the unit and exponent of the Rappture Units object with`
 * the handle 'unitHandle'.
 * Return the unit and exponent as one concatinated string.
 * Error code, err=0 on success, anything else is failure.
 */

void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{
    int            unitsHandle = 0;
    int            err         = 1;
    const RpUnits* unitsObj    = NULL;
    const char*    retString   = NULL;

    /* Check for proper number of arguments. */
    if (nrhs != 1)
        mexErrMsgTxt("Two input required.");
    else if (nlhs > 2)
        mexErrMsgTxt("Too many output arguments.");

    unitsHandle = getIntInput(prhs[0]);

    /* Call the C subroutine. */
    if (unitsHandle > 0) {
        unitsObj = getObject_UnitsStr(unitsHandle);
        if (unitsObj) {
            retString = unitsObj->getUnitsName().c_str();
            if (retString) {
                err = 0;
            }
        }
    }

    /* Set C-style string output_buf to MATLAB mexFunction output*/
    plhs[0] = mxCreateString(retString);
    plhs[1] = mxCreateDoubleScalar(err);

    return;
}
