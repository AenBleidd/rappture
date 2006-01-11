/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    [nodeHandle,err] = rpLibChildren(libHandle,path,prevNodeHandle)
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
// METHOD: [nodeHandle,err] = rpLibChildren(libHandle,path,prevNodeHandle)
/// Retrieve the children of the node described by 'path'
/**
 * This method searches the Rappture Library Object 'libHandle' for the
 * node at the location described by the path 'path' and returns its
 * children. If 'prevNodeHandle' = 0, then the`
 * first child is returned, else, the next child is retrieved.
 * If 'prevNodeHandle' is an invalid child handle, an error will be returned.
 * Subsequent calls to rpLibChildren() should use previously`
 * returned 'nodeHandle's for it 'prevNodeHandle' argument.
 * Error code, err=0 on success, anything else is failure.
 */


void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{
    int         libIndex = 0;
    int         childIndex = 0;
    int         retLibIndex = 0;
    int         err = 1;
    RpLibrary*  lib = NULL;
    RpLibrary*  child = NULL;
    RpLibrary*  retLib = NULL;
    char*       path = NULL;

    /* Check for proper number of arguments. */
    if (nrhs != 3)
        mexErrMsgTxt("Three input required.");
    else if (nlhs > 2)
        mexErrMsgTxt("Too many output arguments.");

    libIndex = getIntInput(prhs[0]);
    path = getStringInput(prhs[1]);
    childIndex = getIntInput(prhs[2]);

    /* Call the C subroutine. */
    if ( (libIndex > 0) && (path) ) {
        lib = getObject_Lib(libIndex);

        if (childIndex > 0) {
            child = getObject_Lib(childIndex);
        }

        if (lib) {
            retLib = rpChildren(lib,path,child);
            retLibIndex = storeObject_Lib(retLib);
            if (retLibIndex) {
                err = 0;
            }
        }
    }

    /* Set double scalar node handle to MATLAB mexFunction output*/
    plhs[0] = mxCreateDoubleScalar(retLibIndex);
    plhs[1] = mxCreateDoubleScalar(err);

    return;
}
