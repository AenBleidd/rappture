/*
 * ----------------------------------------------------------------------
 *  INTERFACE: R Rappture Units Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005-2011  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include <R.h>
#include <Rinternals.h>

#ifdef __cplusplus
extern "C" {
#endif

SEXP RPRUnitsConvertDouble( SEXP fromVal, SEXP toUnitsName) ;
SEXP RPRUnitsConvertString( SEXP fromVal, SEXP toUnitsName, SEXP showUnits) ;

#ifdef __cplusplus
}
#endif // ifdef __cplusplus

