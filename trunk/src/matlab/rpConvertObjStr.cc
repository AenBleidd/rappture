/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    (retStr,result) = 
 *          rpConvertObjStr(fromObjHandle, toObjHandle, value, showUnits)
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
    int         result        = 0;
    std::string retStr        = "";
    int         fromObjHandle = 0;
    int         toObjHandle   = 0;
    RpUnits*    fromObj       = NULL;
    RpUnits*    toObj         = NULL;
    double      value         = 0;
    int         showUnits     = 0;

    /* Check for proper number of arguments. */
    if (nrhs != 4)
        mexErrMsgTxt("Two input required.");
    else if (nlhs > 2)
        mexErrMsgTxt("Too many output arguments.");

    fromObjHandle = getIntInput(prhs[0]);
    toObjHandle   = getIntInput(prhs[1]);
    value         = getDoubleInput(prhs[2]);
    showUnits     = getIntInput(prhs[3]);

    // grab the RpUnits objects from the dictionary
    fromObj       = getObject_UnitsStr(fromObjHandle);
    toObj         = getObject_UnitsStr(toObjHandle);

    /* Call the C subroutine. */
    if (fromObj && toObj) {
        retStr = fromObj->convert(toObj,value,showUnits,&result);
    }

    /* Set C-style string output_buf to MATLAB mexFunction output*/
    plhs[0] = mxCreateString(retStr.c_str());
    plhs[1] = mxCreateDoubleScalar((double)result);

    return;
}
