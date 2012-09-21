/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    [retStr,result] = 
 *          rpUnitsConvertObjStr(fromObjHandle, toObjHandle, value, showUnits)
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
// METHOD: [retStr,err] = rpUnitsConvertObjStr(fromObjHandle,toObjHandle,value,showUnits)
/// Convert between RpUnits return a string value with or without units
/**
 * Convert @var{value} from the units represented by the RpUnits object
 * @var{fromObjHandle} to the units represented by the RpUnits object
 * @var{toObjHandle}. If @var{showUnits} is set to 0, no units will be
 * displayed in @var{retStr}, else units are displayed.`
 * On success, the converted value is returned through
 * @var{retStr}. The second return value, @var{err}, specifies whether`
 * there was an error during conversion.
 * Error code, err=0 on success, anything else is failure.
 */

void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{
    int            result        = 0;
    std::string    retStr        = "";
    int            fromObjHandle = 0;
    int            toObjHandle   = 0;
    const RpUnits* fromObj       = NULL;
    const RpUnits* toObj         = NULL;
    double         value         = 0;
    int            showUnits     = 0;

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
