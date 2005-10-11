/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    rpPutDouble(libHandle,path,value,append)
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
    int         libIndex = 0;
    int         append = 0;
    RpLibrary*  lib = NULL;
    RpLibrary*  retLib = NULL;
    char*       path = NULL;
    double       value = 0.0;

    /* Check for proper number of arguments. */
    if (nrhs != 4)
        mexErrMsgTxt("Two input required.");
    else if (nlhs > 0)
        mexErrMsgTxt("Too many output arguments.");

    libIndex = getIntInput(prhs[0]);
    path = getStringInput(prhs[1]);
    value = getDoubleInput(prhs[2]);
    append = getIntInput(prhs[4]);

    /* Call the C subroutine. */
    if ( (libIndex > 0) && path ) {
        lib = getObject_Lib(libIndex);
        if (lib) {
            rpPutDouble(lib,path,value,append);
        }
    }

    return;
}
