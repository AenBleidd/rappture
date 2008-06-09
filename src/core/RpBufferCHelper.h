/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Rappture Buffer C Helper Header
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2007  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef _RAPPTURE_BUFFER_C_HELPER_H
#define _RAPPTURE_BUFFER_C_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

int RpBufferToCBuffer(Rappture::Buffer* rpbuf, RapptureBuffer* buf);

#ifdef __cplusplus
}
#endif // ifdef __cplusplus

#endif // ifndef _RAPPTURE_BUFFER_C_HELPER_H
