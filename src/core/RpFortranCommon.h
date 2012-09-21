/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Fortran Rappture Common Functions
 *
 *    Fortran functions common to all interfaces.
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include <string>

#ifndef _RpFORTRAN_COMMON_H
#define _RpFORTRAN_COMMON_H

std::string null_terminate_str(const char* inStr, int len);

#ifdef __cplusplus
extern "C" {
#endif

char* null_terminate(char* inStr, int len);
void fortranify(const char* inBuff, char* retText, int retTextLen);

#ifdef __cplusplus
}
#endif
    
#endif
