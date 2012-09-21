/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Interface Helper Functions
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

int
getIntInput ( const mxArray* prhs ) {

    int retVal = -1;
    retVal = (int) getDoubleInput(prhs);
    return retVal;
}

double
getDoubleInput ( const mxArray* prhs ) {

    double retVal = -1;

    /* Input must be a double. */
    if (mxIsDouble(prhs) != 1) {
        mexErrMsgTxt("Input must be a double.");
        return retVal;
    }

    /* Input must be a scalar. */
    if ( (mxGetN(prhs)*mxGetM(prhs)) != 1) {
        mexErrMsgTxt("Input must be a scalar.");
        return retVal;
    }

    retVal = *mxGetPr(prhs);

    return retVal;
}

char*
getStringInput ( const mxArray* prhs ) {

    int         buflen = 0;
    int         status = 0;
    char*       path = NULL;

    /* Input must be a string. */
    if (mxIsChar(prhs) != 1)
        mexErrMsgTxt("Input must be a string.");

    /* Input must be a row vector. */
    if ( (mxGetM(prhs) == 0) && (mxGetN(prhs) == 0)) {
        // do nothin, accept empty strings
    } else if ( (mxGetM(prhs)) != 1) {
        mexErrMsgTxt("Input must be a row vector.");
    }

    /* Get the length of the input string. */
    buflen = (mxGetM(prhs) * mxGetN(prhs)) + 1;

    /* Allocate memory for input and output strings. */
    path = (char*) mxCalloc(buflen, sizeof(char));

    /* Copy the string data from prhs[0] into a C string
     * path. */
    status = mxGetString(prhs, path, buflen);
    if (status != 0)
        mexWarnMsgTxt("Not enough space. String is truncated.");

    return path;
}

void
freeStringInput (void* ptr) {
    if (ptr != NULL) {
        mxFree(ptr);
    }
    ptr = NULL;
}

void
rpmxFlush() {
    mxArray * emptyStr = mxCreateString("");
    mexCallMATLAB(0,0,1,&emptyStr,"fprintf");
}

