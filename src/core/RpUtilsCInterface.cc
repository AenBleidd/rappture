/*
 * ----------------------------------------------------------------------
 *  INTERFACE: C Rappture Utils Source
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
#include "RpUtilsCInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

int
rpUtilsProgress (int percent, const char* text)
{
    return Rappture::Utils::progress(percent,text);
}

#ifdef __cplusplus
}
#endif
