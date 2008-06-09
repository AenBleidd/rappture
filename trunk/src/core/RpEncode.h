/*
 * ======================================================================
 *  Rappture::encoding
 *
 *  AUTHOR:  Derrick Kearney, Purdue University
 *
 *  Copyright (c) 2004-2007  Purdue Research Foundation
 * ----------------------------------------------------------------------
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef RAPPTURE_ENCODE_H
#define RAPPTURE_ENCODE_H

#include <RpBuffer.h>
#include <RpOutcome.h>

#define RPENC_Z      1
#define RPENC_B64    2
#define RPENC_HDR    4

#ifdef __cplusplus
extern "C" {
namespace Rappture {
    namespace encoding {
#endif // ifdef __cplusplus

int isbinary(const char* buf, int size);
Rappture::Outcome encode (Rappture::Buffer& buf, int flags);
Rappture::Outcome decode (Rappture::Buffer& buf, int flags);

#ifdef __cplusplus
    } // namespace encoding
} // namespace Rappture
} // extern C
#endif // ifdef __cplusplus

#endif // RAPPTURE_ENCODE_H
