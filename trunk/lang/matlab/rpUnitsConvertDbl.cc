/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    [retVal,result] = rpUnitsConvertDbl(fromVal, toUnitsName)
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
// METHOD: [retVal,err] = rpUnitsConvertDbl(fromVal,toUnitsName)
/// Convert between RpUnits return a double value without units
/**
 * Convert the value and units in the string @var{fromVal} to units specified
 * in string @var{toUnitsName}. Units will not be shown in @var{retVal}.
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
    double      retVal      = 0;

    /* Check for proper number of arguments. */
    if (nrhs != 2)
        mexErrMsgTxt("Two input required.");
    else if (nlhs > 2)
        mexErrMsgTxt("Too many output arguments.");

    fromVal      = getStringInput(prhs[0]);
    toUnitsName  = getStringInput(prhs[1]);

    /* Call the C subroutine. */
    if (fromVal && toUnitsName) {
        retStr = RpUnits::convert(fromVal,toUnitsName,showUnits,&result);

        if ( !retStr.empty() ) {
            retVal = atof(retStr.c_str());
        }
    }

    /* Set C-style string output_buf to MATLAB mexFunction output*/
    plhs[0] = mxCreateDoubleScalar(retVal);
    plhs[1] = mxCreateDoubleScalar((double)result);

    return;
}
