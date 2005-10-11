/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Interface Helper Functions
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
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
    if ( (mxGetM(prhs)) != 1)
        mexErrMsgTxt("Input must be a row vector.");

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
