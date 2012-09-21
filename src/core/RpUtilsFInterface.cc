/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Fortran Rappture Utils Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpUtils.h"
#include "RpUtilsFInterface.h"
#include "RpUtilsFStubs.c"
#include "RpFortranCommon.h"

#ifdef __cplusplus
extern "C" int rp_utils_progress(int *percent, char *text, int text_len);
#endif

#include <stdlib.h>

/**********************************************************************/
// FUNCTION: rp_utils_progress(int percent, const char* text, int text_len)
/// Report the progress of the application.
/**
 */

int 
rp_utils_progress(int *percentPtr, char *text, int text_len)
{
    const char* inText;
    int retVal;
    inText = null_terminate(text, text_len);
    retVal = Rappture::Utils::progress(*percentPtr, inText);
    free((void*)inText);
    inText = NULL;
    return retVal;
}

