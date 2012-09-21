/*
 * ======================================================================
 *  Rappture::Utils
 *
 *  AUTHOR:  Derrick Kearney, Purdue University
 *
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 * ----------------------------------------------------------------------
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef RAPPTURE_UTILS_H
#define RAPPTURE_UTILS_H

#ifdef __cplusplus
extern "C" {
namespace Rappture {
    namespace Utils {
#endif // ifdef __cplusplus


int progress(int percent, const char* text);


#ifdef __cplusplus
    } // namespace Utils
} // namespace Rappture
} // extern C
#endif // ifdef __cplusplus

#endif // RAPPTURE_UTILS_H
