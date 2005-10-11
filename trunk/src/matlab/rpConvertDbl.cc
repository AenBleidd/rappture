/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    (retVal,result) = rpConvertDbl(fromVal, toUnitsName)
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
