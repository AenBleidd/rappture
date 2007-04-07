/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Rappture-Matlab Bindings Header
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2005  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef _Rp_MATLAB_HELPER_H 
#define _Rp_MATLAB_HELPER_H 

#include "rappture.h"
#include "RpBindingsDict.h"

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
