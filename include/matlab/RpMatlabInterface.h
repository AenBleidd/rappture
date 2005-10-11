/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Rappture-Matlab Bindings Header
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#ifndef _Rp_MATLAB_HELPER_H 
#define _Rp_MATLAB_HELPER_H 

#include "RpBindingsDict.h"
#include "RpLibraryCInterface.h"
#include "RpUnits.h"

// dont delete this, still working on making it happen
//#include "rappture.h"

// include the matlab api header
#include "mex.h"

#ifdef __cplusplus
extern "C" {
#endif

    int     getIntInput     ( const mxArray* prhs );
    double  getDoubleInput  ( const mxArray* prhs );
    char*   getStringInput  ( const mxArray* prhs );

#ifdef __cplusplus
}
#endif

#endif // _Rp_MATLAB_HELPER_H
