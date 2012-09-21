/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Fortran Rappture Utils Header
 *
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef _RAPPTURE_UTILS_F_H
#define _RAPPTURE_UTILS_F_H

#ifdef __cplusplus
    #include "RpUtilsFStubs.h"
    extern "C" {
#endif // ifdef __cplusplus

int rp_utils_progress ( int* percent, char* text, int text_len );


/**********************************************************/

#ifdef __cplusplus
    }
#endif // ifdef __cplusplus

#endif // ifndef _RAPPTURE_UTILS_F_H
