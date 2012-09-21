/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    [retVal,result] = rpUnitsConvertObjDbl(fromObjHandle, toObjHandle, value)
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
// METHOD: [retVal,err] = rpUnitsConvertObjDbl(fromObjHandle,toObjHandle,value)
/// Convert between RpUnits return a double value without units
/**
 * Convert @var{value} from the units represented by the RpUnits object
 * @var{fromObjHandle} to the units represented by the RpUnits object
 * @var{toObjHandle}. On success, the converted value is returned through
 * @var{retVal}. The second return value, @var{err}, specifies whether`
 * there was an error during conversion.
 * Error code, err=0 on success, anything else is failure.
 */

void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{
    int               result        = 0;
    double            retVal        = 0;
    int               fromObjHandle = 0;
    int               toObjHandle   = 0;
    const RpUnits*    fromObj       = NULL;
    const RpUnits*    toObj         = NULL;
    double            value         = 0;

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
