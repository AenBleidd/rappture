/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    (retVal,result) = rpConvertObjDbl(fromObjHandle, toObjHandle, value)
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
    double      retVal        = 0;
    int         fromObjHandle = 0;
    int         toObjHandle   = 0;
    RpUnits*    fromObj       = NULL;
    RpUnits*    toObj         = NULL;
    double      value         = 0;

    /* Check for proper number of arguments. */
    if (nrhs != 3)
        mexErrMsgTxt("Two input required.");
    else if (nlhs > 2)
        mexErrMsgTxt("Too many output arguments.");

    fromObjHandle = getIntInput(prhs[0]);
    toObjHandle   = getIntInput(prhs[1]);
    value         = getDoubleInput(prhs[2]);

    // grab the RpUnits objects from the dictionary
    if ( (fromObjHandle > 0) && (toObjHandle > 0) ){
        fromObj       = getObject_UnitsStr(fromObjHandle);
        toObj         = getObject_UnitsStr(toObjHandle);

        /* Call the C subroutine. */
        if (fromObj && toObj) {
            retVal = fromObj->convert(toObj,value,&result);
        }
    }

    /* Set C-style string output_buf to MATLAB mexFunction output*/
    plhs[0] = mxCreateDoubleScalar(retVal);
    plhs[1] = mxCreateDoubleScalar((double)result);

    return;
}
