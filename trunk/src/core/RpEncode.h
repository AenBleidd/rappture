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

#include <RpOutcome.h>
#include <RpBuffer.h>

namespace Rappture {
namespace encoding {

#define RPENC_Z      (1<<0)
#define RPENC_B64    (1<<1)
#define RPENC_HDR    (1<<2)

int isbinary(const char* buf, int size);
size_t isencoded(const char* buf, int size);
bool encode(Rappture::Outcome &err, Rappture::Buffer& buf, size_t flags);
bool decode(Rappture::Outcome &err, Rappture::Buffer& buf, size_t flags);

}
}
#endif // RAPPTURE_ENCODE_H
