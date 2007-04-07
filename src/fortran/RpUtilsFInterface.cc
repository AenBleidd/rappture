/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Fortran Rappture Utils Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2007  Purdue Research Foundation
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
extern "C" {
#endif

/**********************************************************************/
// FUNCTION: rp_utils_progress(int percent, const char* text, int text_len)
/// Report the progress of the application.
/**
 */

int rp_utils_progress(int* percent, char* text, int text_len)
{
    int retVal = 1;
    const char* inText = NULL;
    inText = null_terminate(text, text_len);
    if (inText != NULL) {
        retVal = Rappture::Utils::progress(*percent,inText);
        free((void*)inText);
    }
    return retVal;
}

#ifdef __cplusplus
}
#endif
