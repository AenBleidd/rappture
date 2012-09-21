/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    [nodeHandle,err] = rpLibChildrenByType(libHandle,path,prevNodeHandle,type)
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
// METHOD: [nodeHandle,err] = rpLibChildrenByType(libHandle,path,prevNodeHandle,type)
/// Retrieve the children of the node described by 'path' of type 'type'
/**
 * This method searches the Rappture Library Object 'libHandle' for the
 * node at the location described by the path 'path' and returns its
 * children that match the type 'type'. If 'prevNodeHandle' = 0, then the`
 * first child is returned, else, the next child is retrieved.
 * If 'prevNodeHandle' is an invalid child handle, an error will be returned.
 * Subsequent calls to rpLibChildrenByType() should use previously`
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
    std::string path = "";
    std::string type = "";

    /* Check for proper number of arguments. */
    if (nrhs != 4) {
        mexErrMsgTxt("Four input required.");
    }

    libIndex = getIntInput(prhs[0]);
    path = getStringInput(prhs[1]);
    childIndex = getIntInput(prhs[2]);
    type = getStringInput(prhs[3]);

    /* Call the C++ subroutine. */
    if ( (libIndex > 0) && (!path.empty()) && (!type.empty()) ) {
        lib = (RpLibrary*) getObject_Void(libIndex);

        if (childIndex > 0) {
            child = (RpLibrary*) getObject_Void(childIndex);
        }

        if (lib) {
            retLib = lib->children(path,child,type);
            retLibIndex = storeObject_Void((void*)retLib);
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
