/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    (retStr,result) = rpConvert(fromVal, toUnitsName, showUnits)
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
