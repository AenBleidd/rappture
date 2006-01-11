/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    [err] = rpLibPutStringId(libHandle,path,value,id,append)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2005  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpMatlabInterface.h"

/**********************************************************************/
// METHOD: [err] = rpLibPutStringId (libHandle,path,value,id,append)
/// Set the value of a node.
/**
 * Clients use this to set the value of a node.  If the path
 * is not specified, it sets the value for the root node.
 * Otherwise, it sets the value for the element specified
 * by the path.  The value is treated as the text within the`
 * tag at the tail of the path.
 *
 * 'id' is used to set the identifier field for the tag at the`
 * tail of the path.  If the append flag is set to 1, then the`
 * value is appended to the current value.  Otherwise, the`
 * value specified in the function call replaces the current value.
 *
 */


void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{
    int         libIndex = 0;
    int         append = 0;
    int         err = 0;
    RpLibrary*  lib = NULL;
    RpLibrary*  retLib = NULL;
    char*       path = NULL;
    char*       value = NULL;
    char*       id = NULL;

    /* Check for proper number of arguments. */
    if (nrhs != 5)
        mexErrMsgTxt("Two input required.");
    else if (nlhs > 1)
        mexErrMsgTxt("Too many output arguments.");

    libIndex = getIntInput(prhs[0]);
    path = getStringInput(prhs[1]);
    value = getStringInput(prhs[2]);
    id = getStringInput(prhs[3]);
    append = getIntInput(prhs[4]);

    /* Call the C subroutine. */
    if ( (libIndex > 0) && path && value  && id) {
        lib = getObject_Lib(libIndex);

        if (lib) {
            rpPutStringId(lib,path,value,id,append);
            err = 0;
        }
    }

    plhs[0] = mxCreateDoubleScalar(err);

    return;
}
