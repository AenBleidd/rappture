/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    [retStr,err] = rpLibGetString(libHandle,path)
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
// METHOD: [retStr,err] = rpLibGetString(libHandle,path)
/// Query the value of a node.
/**
 * Clients use this to query the value of a node.  If the path
 * is not specified, it returns the value associated with the
 * root node.  Otherwise, it returns the value for the element
 * specified by the path. Values are returned as strings.
 *
 * Error code, err=0 on success, anything else is failure.
 */


void 
mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    int libIndex;
    int err = 1;
    std::string path;

    /* Check for proper number of arguments. */
    if (nrhs != 2) {
        mexErrMsgTxt("wrong # of arguments: "
		     "should be rpLibGetString(lib, path)");
    }

    libIndex = getIntInput(prhs[0]);
    path = getStringInput(prhs[1]);

    /* Call the C subroutine. */
    if ( (libIndex > 0) && (!path.empty()) ) {
	RpLibrary*  lib;

        lib = (RpLibrary*)getObject_Void(libIndex);
        if (lib != NULL) {
	    std::string contents;
            contents = lib->getString(path);

	    mwSize dims[2];
	    dims[0] = 1;			/* Treat as a row vector. */
	    dims[1] = contents.length();
	    plhs[0] = mxCreateCharArray(2, dims);
	    
	    /* Copy character array (may include NULs) into the matlab
	     * array. */
	    const char *src = contents.c_str();
	    mxChar *dest = (mxChar *)mxGetPr(plhs[0]);
	    for (int i = 0; i < contents.length(); i++) {
		dest[i] = src[i];
	    }
	    plhs[1] = mxCreateDoubleScalar(0);
	    return;
	}
    }
    /* Set C-style string output_buf to MATLAB mexFunction output*/
    plhs[0] = mxCreateString("");
    plhs[1] = mxCreateDoubleScalar(err);
    return;
}
