/*
 * ----------------------------------------------------------------------
 *  INTERFACE: R Rappture Library Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
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

SEXP RPRLib (SEXP fname) ;
SEXP RPRLibGetString (SEXP handle, SEXP path) ;
SEXP RPRLibGetDouble (SEXP handle, SEXP path) ;
SEXP RPRLibGetInteger (SEXP handle, SEXP path) ;
SEXP RPRLibGetBoolean (SEXP handle, SEXP path) ;
SEXP RPRLibGetFile (SEXP handle, SEXP path, SEXP fileName) ;
SEXP RPRLibPutString (SEXP handle, SEXP path, SEXP value, SEXP append);
// SEXP RPRLibPutData (SEXP handle, SEXP path, SEXP bytes, SEXP nbytes, SEXP append) ;
SEXP RPRLibPutDouble (SEXP handle, SEXP path, SEXP value, SEXP append) ;
SEXP RPRLibPutFile (SEXP handle, SEXP path, SEXP fname, SEXP compress, SEXP append) ;
SEXP RPRLibResult (SEXP handle) ;

#ifdef __cplusplus
}
#endif // ifdef __cplusplus

