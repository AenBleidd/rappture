/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    [retStr,result] = rpUnitsConvert(fromVal, toUnitsName, showUnits)
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
// METHOD: [retStr,err] = rpUnitsConvert(fromVal,toUnitsName,showUnits)
/// Convert between RpUnits return a string value with or without units
/**
 * Convert the value and units in the string @var{fromVal} to units specified
 * in string @var{toUnitsName}. If @var{showUnits} is set to 1, then show the
 * units in the returned string @var{retStr}, else leave the units off.
 * The second return value @var{err} specifies whether there was an error
 * during conversion.
 * Error code, err=0 on success, anything else is failure.
 */

void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{
    const char* fromVal     = NULL;
    const char* toUnitsName  = NULL;
    int         showUnits   = 0;
    int         result      = 0;
    std::string retStr      = "";

    /* Check for proper number of arguments. */
    if (nrhs != 3)
        mexErrMsgTxt("Two input required.");
    else if (nlhs > 2)
        mexErrMsgTxt("Too many output arguments.");

    fromVal      = getStringInput(prhs[0]);
    toUnitsName  = getStringInput(prhs[1]);
    showUnits    = getIntInput(prhs[2]);

    /* Call the C subroutine. */
    if (fromVal && toUnitsName) {
        retStr = RpUnits::convert(fromVal,toUnitsName,showUnits,&result);
    }

    /* Set C-style string output_buf to MATLAB mexFunction output*/
    plhs[0] = mxCreateString(retStr.c_str());
    plhs[1] = mxCreateDoubleScalar((double)result);

    return;
}
