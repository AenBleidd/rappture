/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    [unitsHandle,err] = rpUnitsDefineUnit(unitSymbol, basisHandle)
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
// METHOD: [unitsHandle,err] = rpUnitsDefineUnit(unitSymbol, basisHandle)
/// Define a new Rappture Units type.
/**
 * Define a new Rappture Units type which can be searched for using
 * @var{unitSymbol} and has a basis of @var{basisHandle}. Because of
 * the way the Rappture Units module parses unit names, complex units must
 * be defined as multiple basic units. See the RpUnits Howto for more
 * information on this topic. A @var{basisHandle} equal to 0 means that
 * the unit being defined should be considered as a basis. Unit names must
 * not be empty strings.
 *
 * The first return value, @var{unitsHandle} represents the handle of the
 * instance of the RpUnits object inside the internal dictionary. On
 * success this value will be greater than zero (0), any other value is
 * represents failure within the function. The second return value
 * @var{err} represents the error code returned from the function.
 *
 * Error code, err=0 on success, anything else is failure.
 */

void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{
    char *unitSymbol = NULL;
    const RpUnits* basis = NULL;
    RpUnits* myUnit = NULL;
    int retHandle = 0;
    int basisHandle = 0;
    int retIndex = 0;
    int err = 1;

    /* Check for proper number of arguments. */
    if (nrhs != 2)
        mexErrMsgTxt("Two input required.");
    else if (nlhs > 2)
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
